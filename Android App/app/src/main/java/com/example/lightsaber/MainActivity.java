package com.example.lightsaber;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.method.KeyListener;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    static SeekBar SB_primaryR, SB_primaryG, SB_primaryB, SB_secondaryR, SB_secondaryG, SB_secondaryB;
    static TextView TV_primaryR, TV_primaryG, TV_primaryB, TV_secondaryR, TV_secondaryG, TV_secondaryB;
    static TextView TV_primaryColor, TV_secondaryColor;
    static Switch soundSwitch;
    static Spinner bladeSpinner;
    Button btnApply, btnConnect, btnLoad;
    static CheckBox cbAutoApply;
    static TextView TV_voltage, TV_temperature, TV_connectionStatus;

    static MainActivity mn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mn = MainActivity.this;

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO); //  disable dark mode


        networkItemsSetup();
        colorPickerSetup();
        soundFontPickerSetup();
        bladeEffectPickerSetup();

        if(savedInstanceState == null) {
            forceUseWifi();
            startListeningThread();
        }
    }

    public void startListeningThread() {
        Thread t = new Thread(() -> {
            while(true) {
                try {
                    int[] message = NetworkHandler.readTCPPacket();
                    if(message == null) {
                        Thread.sleep(100);
                        continue;
                    }

                    if(message[0] == 12) {
                        int primaryR = message[1];
                        int primaryG = message[2];
                        int primaryB = message[3];
                        int secondaryR = message[4];
                        int secondaryG = message[5];
                        int secondaryB = message[6];
                        int soundFontIndex = message[7];
                        int bladeEffectIndex = message[8];
                        setSettings(primaryR, primaryG, primaryB, secondaryR,
                                secondaryG, secondaryB, soundFontIndex, bladeEffectIndex);
                    } else if(message[0] == 15) {
                        int voltageTop8 = message[1];
                        int voltageBottom8 = message[2];
                        double voltage = (voltageTop8 << 8) | voltageBottom8;
                        voltage /= 1000;
                        String voltageStr = String.format("%1.2f V", voltage);
                        System.out.println(voltageTop8 + " " + voltageBottom8);
                        mn.runOnUiThread(() -> updateVoltage(voltageStr));
                    } else if(message[0] == 14) {
                        int temperatureTop8 = message[1];
                        int temperatureBottom8 = message[2];
                        double temperature = (temperatureTop8 << 8) | temperatureBottom8;
                        temperature /= 100;
                        String temperatureStr = String.format("%2.1f ºC", temperature);
                        mn.runOnUiThread(() -> updateTemperature(temperatureStr));
                    }

                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });

        t.start();
    }

    public void networkItemsSetup() {
        cbAutoApply = findViewById(R.id.checkBoxAutoApply);
        btnApply = findViewById(R.id.buttonApply);
        btnConnect = findViewById(R.id.buttonConnect);
        btnLoad = findViewById(R.id.buttonLoad);
        TV_voltage = findViewById(R.id.textViewVoltage);
        TV_temperature = findViewById(R.id.textViewTemperature);
        TV_connectionStatus = findViewById(R.id.textViewConnectionStatus);

        btnApply.setOnClickListener(v -> applyAll());
        btnConnect.setOnClickListener(v -> attemptConnection());
        btnLoad.setOnClickListener(v -> requestSettings());
    }

    public static void updateConnectionStatus(String s) {
        TV_connectionStatus.setText(s);
    }

    public static void updateVoltage(String s) {
        TV_voltage.setText(s);
    }

    public static void updateTemperature(String s) {
        TV_temperature.setText(s);
    }

    public void attemptConnection() {
        NetworkHandler.tcpNetworkSetup();
    }

    /*
    Phone -> TM4C
    Color Set: [1][primary_R][primary_G][primary_B][secondary_R][secondary_G][secondary_B]
    Sound Font Set: [2][font_index]
    Blade Effect Set: [3][blade_effect_index]
    Request Settings: [4]

    TM4C -> Phone
    Current Settings: [12][primary_R][primary_G][primary_B][secondary_R][secondary_G][secondary_B][font_index][blade_effect_index]
    Battery Voltage: [13][top 8 bits of voltage*1000][bottom 8 bits of voltage*1000]
    PCB Temperature: [14][top 8 bits of temperature*100][bottom 8 bits of temperature*100]
    */

    public static void setSettings(int pr, int pg, int pb, int sr, int sg, int sb, int font, int effect) {
        SB_primaryR.setProgress(pr);
        SB_primaryG.setProgress(pg);
        SB_primaryB.setProgress(pb);
        SB_secondaryR.setProgress(sr);
        SB_secondaryG.setProgress(sg);
        SB_secondaryB.setProgress(sb);
        mn.runOnUiThread(() -> soundSwitch.setChecked(font == 1));
        mn.runOnUiThread(() -> bladeSpinner.setSelection(effect));
    }

    public void applyColor(int pr, int pg, int pb, int sr, int sg, int sb) {
        byte[] toSend = {1, (byte)pr, (byte)pg, (byte)pb, (byte)sr, (byte)sg, (byte)sb};
        NetworkHandler.sendTCP(toSend);
    }

    public void applySoundFont(int font) {
        byte[] toSend = {2, (byte)font};
        NetworkHandler.sendTCP(toSend);
    }

    public void applyBladeEffect(int effectIndex) {
        byte[] toSend ={3, (byte)effectIndex};
        NetworkHandler.sendTCP(toSend);
    }

    public void requestSettings() {
        byte[] toSend = {4};
        NetworkHandler.sendTCP(toSend);
    }

    public void applyAll() {
        applyColor(SB_primaryR.getProgress(), SB_primaryG.getProgress(), SB_primaryB.getProgress(),
                SB_secondaryR.getProgress(), SB_secondaryG.getProgress(), SB_secondaryB.getProgress());
        applySoundFont(soundSwitch.isChecked() ? 1 : 0);
        applyBladeEffect(bladeSpinner.getSelectedItemPosition());
    }

    boolean runOnce = false;
    public void bladeEffectPickerHandler(int effectIndex) {
        if(!runOnce) {
            //  this janky code ignores the first time this is called, which is on startup
            runOnce = true;
            return;
        }

        if(cbAutoApply.isChecked())
            applyBladeEffect(effectIndex);
    }

    public void soundFontHandler(boolean useAlternativeSoundFont) {
        if(cbAutoApply.isChecked())
            applySoundFont(useAlternativeSoundFont ? 1 : 0);
    }

    @SuppressLint("SetTextI18n")
    public void seekbarHandler() {
        int primaryR = SB_primaryR.getProgress();
        int primaryG = SB_primaryG.getProgress();
        int primaryB = SB_primaryB.getProgress();
        int secondaryR = SB_secondaryR.getProgress();
        int secondaryG = SB_secondaryG.getProgress();
        int secondaryB = SB_secondaryB.getProgress();

        TV_primaryR.setText(""+primaryR);
        TV_primaryG.setText(""+primaryG);
        TV_primaryB.setText(""+primaryB);
        TV_secondaryR.setText(""+secondaryR);
        TV_secondaryG.setText(""+secondaryG);
        TV_secondaryB.setText(""+secondaryB);

        TV_primaryColor.setBackgroundColor(Color.rgb(primaryR, primaryG, primaryB));
        TV_secondaryColor.setBackgroundColor(Color.rgb(secondaryR, secondaryG, secondaryB));

        if(cbAutoApply.isChecked())
            applyColor(primaryR, primaryG, primaryB, secondaryR, secondaryG, secondaryB);
    }

    void bladeEffectPickerSetup() {
        bladeSpinner = findViewById(R.id.spinner);
        bladeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                bladeEffectPickerHandler(position);
            }
            @Override public void onNothingSelected(AdapterView<?> parent) {}
        });
    }

    void soundFontPickerSetup() {
        soundSwitch = findViewById(R.id.soundSwitch);
        soundSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> soundFontHandler(isChecked));
    }

    public void colorPickerSetup() {
        //  find seekbars
        SB_primaryR = findViewById(R.id.seekBarPR);
        SB_primaryG = findViewById(R.id.seekBarPG);
        SB_primaryB = findViewById(R.id.seekBarPB);
        SB_secondaryR = findViewById(R.id.seekBarSR);
        SB_secondaryG = findViewById(R.id.seekBarSG);
        SB_secondaryB = findViewById(R.id.seekBarSB);

        //  find textviews
        TV_primaryR = findViewById(R.id.textViewPR);
        TV_primaryG = findViewById(R.id.textViewPG);
        TV_primaryB = findViewById(R.id.textViewPB);
        TV_secondaryR = findViewById(R.id.textViewSR);
        TV_secondaryG = findViewById(R.id.textViewSG);
        TV_secondaryB = findViewById(R.id.textViewSB);
        TV_primaryColor = findViewById(R.id.textViewPrimaryColor);
        TV_secondaryColor = findViewById(R.id.textViewSecondaryColor);

        // perform seek bar change listener event used for getting the progress value
        SB_primaryR.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        SB_primaryG.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        SB_primaryB.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        SB_secondaryR.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        SB_secondaryG.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        SB_secondaryB.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) { seekbarHandler(); }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

    }

    //  forces the use of wifi instead of mobile data for socket connections
    private void forceUseWifi() {
        final ConnectivityManager manager = (ConnectivityManager) getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkRequest.Builder builder;
        builder = new NetworkRequest.Builder();
        //set the transport type do WIFI
        builder.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        manager.requestNetwork(builder.build(), new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                manager.bindProcessToNetwork(network);
//                try {
//                    //do a callback or something else to alert your code that it's ok to send the message through socket now
//                } catch (Exception e) {
//                    e.printStackTrace();
//                }
                manager.unregisterNetworkCallback(this);
            }
        });
    }

}