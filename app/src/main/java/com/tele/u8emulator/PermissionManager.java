package com.tele.u8emulator;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import java.util.ArrayList;
import java.util.List;

public class PermissionManager {
    private static final String TAG = "PermissionManager";
    public static final int PERMISSION_REQUEST_CODE = 123;
    public static final int MANAGE_STORAGE_REQUEST_CODE = 124;

    public static boolean checkAndRequestPermissions(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                try {
                    Log.d(TAG, "Requesting MANAGE_EXTERNAL_STORAGE permission");
                    Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                    Uri uri = Uri.parse("package:" + activity.getPackageName());
                    intent.setData(uri);
                    activity.startActivityForResult(intent, MANAGE_STORAGE_REQUEST_CODE);
                    return false;
                } catch (Exception e) {
                    Log.e(TAG, "Error requesting MANAGE_EXTERNAL_STORAGE: " + e.getMessage());
                    Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    activity.startActivityForResult(intent, MANAGE_STORAGE_REQUEST_CODE);
                    return false;
                }
            }
            return true;
        }
        else {
            List<String> permissionsNeeded = new ArrayList<>();

            String[] requiredPermissions = {
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            };
            
            for (String permission : requiredPermissions) {
                if (ContextCompat.checkSelfPermission(activity, permission) 
                    != PackageManager.PERMISSION_GRANTED) {
                    permissionsNeeded.add(permission);
                }
            }

            if (!permissionsNeeded.isEmpty()) {
                Log.d(TAG, "Requesting standard storage permissions");
                ActivityCompat.requestPermissions(activity,
                    permissionsNeeded.toArray(new String[0]),
                    PERMISSION_REQUEST_CODE);
                return false;
            }
            
            return true;
        }
    }

    public static boolean hasAllPermissions(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            String[] requiredPermissions = {
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            };
            
            for (String permission : requiredPermissions) {
                if (ContextCompat.checkSelfPermission(activity, permission) 
                    != PackageManager.PERMISSION_GRANTED) {
                    return false;
                }
            }
            return true;
        }
    }
}