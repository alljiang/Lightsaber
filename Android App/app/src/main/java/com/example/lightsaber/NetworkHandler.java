package com.example.lightsaber;

import android.os.Looper;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;

public class NetworkHandler {

//    static String IP = "192.168.0.198";
    static String IP = "192.168.4.1";
    static int port = 333;
    // TCP
    static boolean tcpConnectionInProgress;
    static Socket tcpSocket;
    static InputStream tcpInputStream;
    static BufferedReader tcpInput;
    static OutputStream tcpOutputStream;

    static boolean sendTCP(byte[] buffer) {
        if(tcpSocket == null) {
            MainActivity.mn.runOnUiThread(() -> MainActivity.updateConnectionStatus("Transmit Failed: Not connected"));
            return false;
        }
        new Thread(() -> {
            try {
                tcpOutputStream.write(buffer);
                tcpOutputStream.flush();
                StringBuilder s = new StringBuilder();
                for (byte c : buffer) s.append(c);
                Log.i("TCP", "Sending: \"" + s + "\"");
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
        MainActivity.mn.runOnUiThread(() -> MainActivity.updateConnectionStatus("Transmit Successful"));
        return true;
    }

    static int[] readTCPPacket() {
        if(tcpSocket == null) return null;
        try {
            if(!tcpSocket.isConnected()) {
                Log.i("TCP", "nope");
                return null;
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            System.out.println("Awaiting Packet...");
            int header = tcpInputStream.read();

            int qtyToRead = 0;
            if(header == 12) qtyToRead = 8;
            else if(header == 15) qtyToRead = 2;
            else if(header == 14) qtyToRead = 2;

            System.out.println("header: " + header);

            int[] packet = new int[1+qtyToRead];
            packet[0] = header;
            for(int i = 1; i <= qtyToRead; i++) {
                packet[i] = tcpInputStream.read();
            }
            return packet;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    static public void tcpNetworkSetup() {
        if(tcpConnectionInProgress || (tcpSocket != null && tcpSocket.isConnected())) return;

        Log.i("TCPNetworkSetupThread", "tryna do a TCP setup");
        MainActivity.updateConnectionStatus("Attempting connection...");
        tcpConnectionInProgress = true;
        new Thread(() -> {
            Looper.prepare();
            try {
                tcpSocket = new Socket();
                tcpSocket.connect(new InetSocketAddress(IP, port), 1000);
                tcpInputStream = tcpSocket.getInputStream();
                tcpInput = new BufferedReader(new InputStreamReader(tcpInputStream));
                tcpOutputStream = tcpSocket.getOutputStream();

                Log.i("TCPNetworkSetupThread", "TCP is gucci");
                MainActivity.mn.runOnUiThread(() -> MainActivity.updateConnectionStatus("Connection Established"));
            } catch (SocketTimeoutException e) {
                Log.i("TCPNetworkSetupThread", "TCP connection timed out");
                MainActivity.mn.runOnUiThread(() -> MainActivity.updateConnectionStatus("Timed Out"));
            } catch(SocketException e) {
                Log.i("TCPNetworkSetupThread", "Connection Failed");
                MainActivity.mn.runOnUiThread(() -> MainActivity.updateConnectionStatus("Connection Failed"));
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            tcpConnectionInProgress = false;
        }).start();
    }

}