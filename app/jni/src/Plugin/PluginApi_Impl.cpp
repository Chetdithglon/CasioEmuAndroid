#define _NO_FUND_API
#include "PluginApi.h"
#include <CPU.hpp>
#include <Chipset.hpp>
#include <MMU.hpp>
extern std::vector<UIWindow*> windows;

class PluginApi_Impl : public PluginApi {

	class IMMU_Impl : public IMMU {
		uint8_t ReadData(size_t addr) override {
			return me_mmu->ReadData(addr);
		}
		void WriteData(size_t addr, uint8_t dat) override {
			me_mmu->WriteData(addr, dat);
		}
		uint16_t ReadCode(size_t addr) override {
			return me_mmu->ReadCode(addr);
		}
		void WriteCode(size_t addr, uint8_t dat) override {
			m_emu->chipset.rom_data[addr] = dat;
		}
	} mmu_impl;
	class ICPU_Impl : public ICPU {
		// ͨ�� ICPU �̳�
		uint16_t* Register(const char* name) override {
			return &m_emu->chipset.cpu.register_proxies[name]->raw;
		}
	} cpu_impl;
	class IEmulator_Impl : public IEmulator {
		// ͨ�� IEmulator �̳�
		float* SolarPanelVoltage() override {
			return &m_emu->SolarPanelVoltage;
		}
		float* BatteryVoltage() override {
			return &m_emu->BatteryVoltage;
		}
		casioemu::ModelInfo* ModelDefinition() override {
			return &m_emu->ModelDefinition;
		}
		void RequestScreenshot() override {
			m_emu->screenshot_requested.store(true);
		}
		bool IsPaused() override {
			return m_emu->GetPaused();
		}
		void Pause() override {
			m_emu->SetPaused(true);
		}
		void Resume() override {
			m_emu->SetPaused(false);
		}
		unsigned int GetCyclesPerSecond() override {
			return m_emu->cycles.cycles_per_second;
		}
		void SetCyclePerSecond(uint32_t cps) override {
			m_emu->cycles.cycles_per_second = cps;
		}
		void* GetRenderer() override {
			return m_emu->renderer;
		}
		void* GetInterfaceTexture() override {
			return m_emu->interface_texture;
		}
		std::string GetModelFilePath(std::string relative_path) override {
			return m_emu->GetModelFilePath(relative_path);
		}
	} emu_impl;
	class IChipset_Impl : public IChipset {
		// ͨ�� IChipset �̳�
		void RaiseInterrupt(int index) override {
			m_emu->chipset.RaiseMaskable(index);
		}
		void Tick() override {
			m_emu->chipset.Tick();
		}
		void SetStatus(RunStatus status) override {
			m_emu->chipset.run_mode = (casioemu::Chipset::RunMode)status;
		}
		RunStatus GetStatus() override {
			return RunStatus(m_emu->chipset.run_mode);
		}
		void* GetRom() {
			return m_emu->chipset.rom_data.data();
		}
		size_t GetRomSize() {
			return m_emu->chipset.rom_data.size();
		}
	} chipset_impl;
	class Hooks_Impl : public Hooks {
		// ͨ�� Hooks �̳�

		// ע��ָ��ִ�� hook������� handler ֻ��Ҫ���� InstructionEventArgs
		void SetupOnInstructionHook(std::function<void(InstructionEventArgs&)> handler) override {
			SetupHook(on_instruction,
				[handler](casioemu::CPU& /*cpu*/, InstructionEventArgs& args) {
					handler(args);
				});
		}

		// ע�ắ������ hook������� handler ֻ��Ҫ���� FunctionEventArgs
		void SetupOnCallFunctionHook(std::function<void(const FunctionEventArgs&)> handler) override {
			SetupHook(on_call_function,
				[handler](casioemu::CPU& /*cpu*/, const FunctionEventArgs& args) {
					handler(args);
				});
		}

		// ע�ắ������ hook������� handler ֻ��Ҫ���� FunctionEventArgs
		void SetupOnFunctionReturnHook(std::function<void(const FunctionEventArgs&)> handler) override {
			SetupHook(on_function_return,
				[handler](casioemu::CPU& /*cpu*/, const FunctionEventArgs& args) {
					handler(args);
				});
		}

		// ע���ڴ��ȡ hook������� handler ֻ��Ҫ���� MemoryEventArgs
		void SetupOnMemoryReadHook(std::function<void(MemoryEventArgs&)> handler) override {
			SetupHook(on_memory_read,
				[handler](casioemu::MMU& /*mmu*/, MemoryEventArgs& args) {
					handler(args);
				});
		}

		// ע���ڴ�д�� hook������� handler ֻ��Ҫ���� MemoryEventArgs
		void SetupOnMemoryWriteHook(std::function<void(MemoryEventArgs&)> handler) override {
			SetupHook(on_memory_write,
				[handler](casioemu::MMU& /*mmu*/, MemoryEventArgs& args) {
					handler(args);
				});
		}

		// ע���ж϶ϵ� hook������� handler ֻ��Ҫ���� InterruptEventArgs
		void SetupOnBrkHook(std::function<void(InterruptEventArgs&)> handler) override {
			SetupHook(on_brk,
				[handler](casioemu::Chipset& /*chipset*/, InterruptEventArgs& args) {
					handler(args);
				});
		}

		// ע���ж� hook������� handler ֻ��Ҫ���� InterruptEventArgs
		void SetupOnInterruptHook(std::function<void(InterruptEventArgs&)> handler) override {
			SetupHook(on_interrupt,
				[handler](casioemu::Chipset& /*chipset*/, InterruptEventArgs& args) {
					handler(args);
				});
		}

		// ע�Ḵλ hook������� handler �޲��������ڲ� hook ���� Chipset ����
		void SetupOnResetHook(std::function<void()> handler) override {
			SetupHook(on_reset,
				[handler](casioemu::Chipset& /*chipset*/) {
					handler();
				});
		}
	} hooks_impl;
	int GetVersion() override {
		return 1;
	}
	void AddWindow(UIWindow* wnd) override {
		windows.push_back(wnd);
	}
	bool RegisterPlugin(const char* id, const char* name, int version) override {
		std::cout << name << " loaded.\n";
		return true;
	}
	void* QueryInterface(const char* name) override {
		if (strcmp(name, typeid(IEmulator).name()) == 0)
			return &emu_impl;
		if (strcmp(name, typeid(ICPU).name()) == 0)
			return &cpu_impl;
		if (strcmp(name, typeid(IChipset).name()) == 0)
			return &chipset_impl;
		if (strcmp(name, typeid(IMMU).name()) == 0)
			return &mmu_impl;
		if (strcmp(name, typeid(Hooks).name()) == 0)
			return &hooks_impl;
		return m_emu->chipset.QueryInterface(name);
	}
};
PluginApi* g_pluginapi = new PluginApi_Impl();