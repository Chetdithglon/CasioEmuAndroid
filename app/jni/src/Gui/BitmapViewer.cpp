#include "Localization.h"
#include "Ui.hpp"
#include <cmath> // for std::trunc, std::fmod, round

class BitmapViewer : public UIWindow {
public:
	BitmapViewer() : UIWindow("Bitmap") {}

	// ���������ڴ��ַ���ַ�����������ʮ��������ʽ���룩
	char bufaddr[10] = {};
	// ÿ����ʾ����������Ĭ�� 16����ͨ�����������
	int width = 16;
	// ÿ�����ص���ʾ�ߴ磨���ش�С����Ĭ�� 10
	int size = 10;
	// λƫ�ƣ�bit offset������Χ 0��7���������ֽ�����һλ��ʼ��ȡ
	int bitOffset = 0;

	// �����ۼ������ֵı仯
	double wheeldelta = 0;

	void RenderCore() override {
		// ���� 1. �ϲ��������򣺵�ַ���뼰��������
		auto startY = ImGui::GetCursorPosY();
		ImGui::InputText("BitmapViewer.Address"_lc, bufaddr, sizeof(bufaddr));
		// ��������ȡ��ʼ��ַ��ʮ�����ƣ�
		uint32_t addr = static_cast<uint32_t>(strtol(bufaddr, nullptr, 16));
		addr = addr & 0xfffff;
		if (ImGui::InputInt("BitmapViewer.Address_2"_lc, (int*)&addr)) {
			sprintf(bufaddr, "%08X", addr);
		}
		ImGui::SliderInt("BitmapViewer.Width"_lc, &width, 1, 256);
		ImGui::SliderInt("BitmapViewer.PixelSize"_lc, &size, 1, 256);
		ImGui::SliderInt("BitmapViewer.BitOffset"_lc, &bitOffset, 0, 7);
		ImGui::Dummy(ImVec2{0, 20});

		// ��������߶Ⱦ�����ʾ������
		float availHeight = ImGui::GetContentRegionAvail().y;
		int rows = static_cast<int>(availHeight) / size;
		int numPixels = width * rows;

		// ��������ߴ�
		ImVec2 gridSize(width * size, rows * size);
		ImGui::Dummy(gridSize);
		// ��¼ Dummy ��Ӧ�����򣨺������ƾ����ڴ�����
		ImVec2 startPos = ImGui::GetItemRectMin();
		// �������������Ƿ������ͣ�����������������������
		bool gridHovered = ImGui::IsItemHovered();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// ���� ����λͼ���ݣ����ζ�ȡÿ��λ��Ӧ������
		uint8_t currentByte = 0;
		int currentByteIndex = -1;
		for (int i = 0; i < numPixels; ++i) {
			int effectiveBitPos = bitOffset + i;
			int byteIndex = effectiveBitPos / 8;
			int bitIndex = 7 - (effectiveBitPos % 8);

			int col = i % width;
			int row = i / width;
			ImVec2 pixelTopLeft = ImVec2(startPos.x + col * size, startPos.y + row * size);
			ImVec2 pixelBottomRight = ImVec2(pixelTopLeft.x + size, pixelTopLeft.y + size);

			if (byteIndex != currentByteIndex) {
				currentByte = me_mmu->ReadData((addr + byteIndex) & 0xfffff);
				currentByteIndex = byteIndex;
			}
			bool isSet = ((currentByte >> bitIndex) & 1) != 0;
			if (isSet)
				drawList->AddRectFilled(pixelTopLeft, pixelBottomRight, IM_COL32(255, 255, 255, 255));
		}
		ImVec2 endPos = ImVec2(startPos.x + gridSize.x, startPos.y + gridSize.y);
		if (size > 3) {
			// �����������
			drawList->AddRect(startPos, endPos, IM_COL32(200, 200, 200, 255));

			// ���� �����п̶�
			for (int col = 0; col < width; ++col) {
				float x = startPos.x + col * size;
				const int tickLength = 5;
				if (col % 8 == 0) {
					drawList->AddLine(ImVec2(x, startPos.y),
						ImVec2(x, startPos.y - tickLength),
						IM_COL32(255, 255, 255, 255));
					char colLabel[16];
					sprintf(colLabel, "%X", col);
					drawList->AddText(ImVec2(x + 1, startPos.y - tickLength - 12),
						IM_COL32(255, 255, 255, 255), colLabel);
				}
			}
			// ���� �����п̶�
			for (int row = 0; row < rows; ++row) {
				float y = startPos.y + row * size;
				const int tickLength = 5;
				if (row % 8 == 0) {
					drawList->AddLine(ImVec2(endPos.x, y),
						ImVec2(endPos.x + tickLength, y),
						IM_COL32(255, 255, 255, 255));
					char rowLabel[16];
					sprintf(rowLabel, "%06X", (row + addr) & 0xfffff);
					drawList->AddText(ImVec2(endPos.x + tickLength, y - 6),
						IM_COL32(255, 255, 255, 255), rowLabel);
				}
			}
		}
		// ���� 2. ��ӡ���������������ڿ�����ת���˹��������Ǵ��ڵ�ʵ�ʹ�������
		{
			// ʾ�����趨�ڴ淶ΧΪ 1MB���ɸ�����Ҫ������
			constexpr uint32_t MEMORY_SIZE = 0x100000;
			// �ɼ����������λ�����ֽ���������ȡ����
			int visibleBits = width * rows;
			int visibleBytes = (bitOffset + visibleBits + 7) / 8;
			uint32_t maxEffectiveAddr = (MEMORY_SIZE > (uint32_t)visibleBytes) ? MEMORY_SIZE - visibleBytes : 0;

			// �Ը�������ʾ�ġ���Ч��ַ������С�����ֱ�ʾ bitOffset �ı���
			float effectiveAddr = addr + bitOffset / 8.0f;

			// ����������ļ������򣺷��������Ҳ࣬��� 10 ����
			float scrollBarWidth = 16.f;
			ImVec2 scrollBarPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - scrollBarWidth, ImGui::GetWindowPos().y + startY);
			ImVec2 scrollBarSize = ImVec2(scrollBarWidth - 1.0f, ImGui::GetWindowSize().y - startY - 10.0f);

			// ���ƹ���������
			drawList->AddRectFilled(scrollBarPos,
				ImVec2(scrollBarPos.x + scrollBarWidth, scrollBarPos.y + scrollBarSize.y),
				IM_COL32(50, 50, 50, 255));

			// �����������飩�ĸ߶ȣ���ɼ��ֽ���ռ�ڴ淶Χ�ı����й�
			float handleHeight = (visibleBytes / (float)MEMORY_SIZE) * scrollBarSize.y;
			if (handleHeight < 10.0f)
				handleHeight = 10.0f; // ��С�߶�

			// �������ڹ������еĴ�ֱλ��
			float handlePosY = 0.0f;
			if (maxEffectiveAddr > 0)
				handlePosY = (effectiveAddr / maxEffectiveAddr) * (scrollBarSize.y - handleHeight);

			ImVec2 handleMin = ImVec2(scrollBarPos.x, scrollBarPos.y + handlePosY);
			ImVec2 handleMax = ImVec2(scrollBarPos.x + scrollBarWidth, scrollBarPos.y + handlePosY + handleHeight);
			drawList->AddRectFilled(handleMin, handleMax, IM_COL32(150, 150, 150, 255));

			// �� InvisibleButton ��������ڸ������ڵĵ������ק�¼�
			ImGui::SetCursorScreenPos(scrollBarPos);
			ImGui::InvisibleButton("VirtualScrollbar", scrollBarSize);
			bool scrollbarHovered = ImGui::IsItemHovered();
			if (ImGui::IsItemActive()) {
				float mouseY = ImGui::GetIO().MousePos.y;
				float newHandlePosY = mouseY - scrollBarPos.y - handleHeight / 2;
				// ���� newHandlePosY �ں���Χ��
				if (newHandlePosY < 0.0f)
					newHandlePosY = 0.0f;
				if (newHandlePosY > scrollBarSize.y - handleHeight)
					newHandlePosY = scrollBarSize.y - handleHeight;
				float newEffectiveAddr = (scrollBarSize.y - handleHeight > 0)
											 ? (newHandlePosY / (scrollBarSize.y - handleHeight)) * maxEffectiveAddr
											 : 0.0f;
				// ���� newEffectiveAddr ��ԭ���µ� addr �� bitOffset��bitOffset Ϊ newEffectiveAddr ��С�����ֳ� 8��
				int newAddr = (int)newEffectiveAddr;
				int newBitOffset = (int)round((newEffectiveAddr - newAddr) * 8.0f);
				if (newBitOffset >= 8) {
					newAddr++;
					newBitOffset -= 8;
				}
				addr = newAddr;
				bitOffset = newBitOffset;
				sprintf(bufaddr, "%08X", addr);
			}
		}

		// ���� 3. �����ֹ��������������ͣ�����������Ҳ��ڹ�������ʱ����Ӧ
		if (gridHovered && !ImGui::IsItemActive()) {
			wheeldelta += ImGui::GetIO().MouseWheel;
			if (std::abs(wheeldelta) >= 1.0f) {
				int rowDelta = static_cast<int>(std::trunc(wheeldelta));
				wheeldelta = std::fmod(wheeldelta, 1.0f);
				int bitDelta = width * rowDelta;
				int byteDelta = bitDelta / 8;
				int bitRemainder = bitDelta % 8;

				int newAddr = addr + byteDelta;
				int newBitOffset = bitOffset + bitRemainder;
				// ��� bitOffset �����Ͻ磬��λ
				if (newBitOffset > 7) {
					int carry = newBitOffset / 8;
					newAddr += carry;
					newBitOffset %= 8;
				}
				// ��� bitOffset С�� 0�������λ��λ
				else if (newBitOffset < 0) {
					int borrow = (-newBitOffset + 7) / 8;
					newAddr = newAddr > borrow ? newAddr - borrow : 0;
					newBitOffset += borrow * 8;
				}
				addr = newAddr;
				bitOffset = newBitOffset;
				sprintf(bufaddr, "%08X", addr);
			}
		}
		// ���� 4. ��Ӽ��̹����������¼�ͷ��PageUp��PageDown��
		// ��������ӵ�н�����δ�����ı�����ʱ��Ӧ
		if (ImGui::IsWindowFocused()) {
			int rowDelta = 0;
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				rowDelta = -1;
			else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				rowDelta = 1;
			else if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
				rowDelta = -rows;
			else if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
				rowDelta = rows;

			if (rowDelta != 0) {
				int bitDelta = width * rowDelta;
				int byteDelta = bitDelta / 8;
				int bitRemainder = bitDelta % 8;

				int newAddr = addr + byteDelta;
				int newBitOffset = bitOffset + bitRemainder;
				if (newBitOffset > 7) {
					int carry = newBitOffset / 8;
					newAddr += carry;
					newBitOffset %= 8;
				}
				else if (newBitOffset < 0) {
					int borrow = (-newBitOffset + 7) / 8;
					newAddr = newAddr > borrow ? newAddr - borrow : 0;
					newBitOffset += borrow * 8;
				}
				addr = newAddr;
				bitOffset = newBitOffset;
				sprintf(bufaddr, "%08X", addr);
			}
		}
	}
};

UIWindow* CreateBitmapViewer() {
	return new BitmapViewer();
}
