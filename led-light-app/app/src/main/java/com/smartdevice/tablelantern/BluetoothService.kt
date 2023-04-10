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
        private val bleUUID: UUID = UUID.fromString("7b985306-5d4c-4c8b-a143-09b861f2e809")

        private val bleSocket: BluetoothSocket? by lazy(LazyThreadSafetyMode.NONE) {
            device.createRfcommSocketToServiceRecord(bleUUID)
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

    private inner class ConnectedThread(private val mmSocket: BluetoothSocket) : Thread() {
        private val mmInStream: InputStream = mmSocket.inputStream
        private val mmOutStream: OutputStream = mmSocket.outputStream
        private val mmBuffer: ByteArray = ByteArray(1024)

        override fun run() {
            handler.obtainMessage(MESSAGE_CONNECTED)
            var numBytes: Int

            while (true) {
                numBytes = try {
                    mmInStream.read(mmBuffer)
                } catch (e: IOException) {
                    logger.info("Input stream was disconnected")
                    break
                }
                logger.info("Input Stream" + String(mmBuffer))

                // Send the obtained bytes to the UI activity.
                val readMsg = handler.obtainMessage(MESSAGE_READ, numBytes, -1, mmBuffer)
                readMsg.sendToTarget()
            }
        }

        fun write(bytes: ByteArray) {
            try {
                mmOutStream.write(bytes)
            } catch (e: IOException) {
                logger.warning("Error occurred when sending data")
                return
            }

            // Share the sent message with the UI activity.
            val writtenMsg = handler.obtainMessage(MESSAGE_WRITE, -1, -1, mmBuffer)
            writtenMsg.sendToTarget()
        }

        fun cancel() {
            try {
                mmSocket.close()
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

    fun connected(mmSocket: BluetoothSocket?) {
        if (mmSocket != null) {
            logger.warning("BT Connected")
            connectedThread = ConnectedThread(mmSocket)
            connectedThread?.start()
            send("REQ")
        }
    }

    fun send(command: String) {
        connectedThread?.write(command.toByteArray())
    }
}