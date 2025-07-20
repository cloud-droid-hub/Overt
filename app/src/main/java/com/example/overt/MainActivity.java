package com.example.overt;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.Toast;

import tee_info.TEEStatus;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "lxz_MainActivity";
    private DeviceInfoProvider deviceInfoProvider;
    private LinearLayout mainContainer;




    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.e(TAG, "onCreate started");

        try {
            // Set the content view first
            setContentView(R.layout.activity_main);

            TEEStatus.get_tee_info();

            // Initialize main container
            mainContainer = findViewById(R.id.main);
            if (mainContainer == null) {
                Log.e(TAG, "Failed to find main container");
                return;
            }

            // Test our TEE parsing function directly
            Log.e(TAG, "Testing TEE parsing function...");
            deviceInfoProvider = new DeviceInfoProvider(this, mainContainer);
            Log.e(TAG, "TEE parsing test completed");

            Log.e(TAG, "onCreate completed successfully");

        } catch (Exception e) {
            Log.e(TAG, "Error in onCreate", e);
            Toast.makeText(this, "初始化失败: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }


    }
}