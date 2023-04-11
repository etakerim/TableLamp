package com.smartdevice.tablelantern

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.os.Handler
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.util.*
import java.util.logging.Logger


@SuppressLint("MissingPermission")
class BluetoothService(activityHandler: Handler) {
    private val logger = Logger.getLogger(this.javaClass.name)
    private var connectThread: ConnectThread? = null
    private var connectedThread: ConnectedThread? = null
    private var handler: Handler = activityHandler

    private inner class ConnectThread(device: BluetoothDevice) : Thread() {
        // Default UUID:
        // private val bleUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb")

        private val bleSocket: BluetoothSocket? by lazy {
            val lampUUID: UUID = device.uuids[0].uuid
            logger.info("UUID: $lampUUID")
            device.createRfcommSocketToServiceRecord(lampUUID)
        }

        override fun run() {
            logger.info("Connect started")
            try {
                bleSocket?.let {
                    socket -> socket.connect()   // Blocking call
                    connected(bleSocket);
                }
            } catch (e: IOException) {
                logger.warning("Could not connect the client socket")
            }
        }

        fun cancel() {
            try {
                bleSocket?.close()
            } catch (e: IOException) {
                logger.warning("Could not close the client socket")
            }
        }
    }

    private inner class ConnectedThread(private val socket: BluetoothSocket) : Thread() {
        private val inboudStream: InputStream = socket.inputStream
        private val outboundStream: OutputStream = socket.outputStream
        private val buffer: ByteArray = ByteArray(1024)

        override fun run() {
            handler.obtainMessage(MESSAGE_CONNECTED).sendToTarget()
            var numBytes: Int

            while (true) {
                numBytes = try {
                    inboudStream.read(buffer)
                } catch (e: IOException) {
                    logger.info("Input stream was disconnected")
                    break
                }
                handler.obtainMessage(MESSAGE_READ, numBytes, -1, buffer).sendToTarget()
            }
        }

        fun write(bytes: ByteArray) {
            try {
                outboundStream.write(bytes)
            } catch (e: IOException) {
                logger.warning("Error occurred when sending data")
                return
            }
            handler.obtainMessage(MESSAGE_WRITE, -1, -1, buffer).sendToTarget()
        }

        fun cancel() {
            try {
                socket.close()
            } catch (e: IOException) {
                logger.warning( "Could not close the connect socket")
            }
        }
    }


    fun connect(device: BluetoothDevice) {
        logger.warning("Bluetooth started")
        if (connectThread != null) {
            connectThread?.cancel();
            connectThread = null;
        }

        if (connectedThread != null) {
            connectedThread?.cancel();
            connectedThread = null;
        }
        connectThread = ConnectThread(device)
        connectThread?.start()
    }

    fun connected(socket: BluetoothSocket?) {
        if (socket != null) {
            logger.warning("Bluetooth Connected")
            connectedThread = ConnectedThread(socket)
            connectedThread?.start()
            send("REQ")
        }
    }

    fun send(command: String) {
        connectedThread?.write(command.toByteArray())
    }
}