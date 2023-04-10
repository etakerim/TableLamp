@file:Suppress("DEPRECATION")

package com.smartdevice.tablelantern

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.bluetooth.*
import android.content.*
import android.os.Bundle
import android.os.Handler
import android.os.Message
import android.text.InputFilter
import android.text.Spanned
import android.widget.*
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import java.lang.ref.WeakReference
import java.util.*
import java.util.logging.Logger

/*
TODO: odtestovať posielanie príkazov
TODO: príjem a nastavenie stavu (parsovanie správy)
TODO: FW save state to NVS
 */

const val MESSAGE_READ: Int = 0
const val MESSAGE_WRITE: Int = 1
const val MESSAGE_CONNECTED: Int = 3
const val HC05_BT_ADDRESS: String = "98:D3:32:70:D8:97"

// https://github.com/android/connectivity-samples/blob/master/BluetoothChat/Application/src/main/java/com/example/android/bluetoothchat/BluetoothChatService.java
// https://github.com/android/connectivity-samples/blob/master/BluetoothChat/Application/src/main/java/com/example/android/bluetoothchat/BluetoothChatFragment.java

@Suppress("DEPRECATION")
@SuppressLint("MissingPermission")
class MainActivity : AppCompatActivity() {
    private val logger = Logger.getLogger(this.javaClass.name)
    private var bluetoothAdapter: BluetoothAdapter? = null
    private var blService: BluetoothService? = null
    private val bluetoothDevices = ArrayList<BluetoothDevice>()
    private val blHandler = BlHandler(WeakReference<MainActivity>(this))
    private val btWaitDialog: AlertDialog by lazy {
        AlertDialog
            .Builder(this)
            .setTitle(getString(R.string.connect_wait))
            .setNegativeButton(getString(R.string.halt_app)) { _: DialogInterface, _: Int ->
                finish()
            }
            .create()
    }


    @SuppressLint("UseSwitchCompatOrMaterialCode")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val lightSwitchBtn: ToggleButton = findViewById(R.id.light_switch)
        val motionDetectBtn: Switch = findViewById(R.id.motion_detection)
        val applySettingsBtn: Button = findViewById(R.id.apply_btn)

        lightSwitchBtn.setOnCheckedChangeListener { _, _ ->
            logger.info("[CMD] Switch light")
            blService?.send("SWITCH")

        }
        motionDetectBtn.setOnCheckedChangeListener { _, _ ->
            logger.info("[CMD] Change motion detection")
            blService?.send("DETECT")
        }
        applySettingsBtn.setOnClickListener {
            logger.info("[CMD] Apply settings")
            val levelEdit: EditText = findViewById(R.id.brightness)
            val kelvinEdit: EditText = findViewById(R.id.color_temperature)
            val luxEdit: EditText = findViewById(R.id.lux_threshold)

            levelEdit.filters = arrayOf<InputFilter>(MinMaxFilter(0, 100))
            kelvinEdit.filters = arrayOf<InputFilter>(MinMaxFilter(1000, 40000))
            luxEdit.filters = arrayOf<InputFilter>(MinMaxFilter(0, 10000))

            // TODO: temporary /////////////
            levelEdit.setText(100.toString())
            kelvinEdit.setText(4000.toString())
            luxEdit.setText(50.toString())
            ///////////////////////////////

            val level: Int = levelEdit.text.toString().toInt()
            val kelvin: Int = kelvinEdit.text.toString().toInt()
            val lux: Int = luxEdit.text.toString().toInt()

            blService?.send("LEVEL ${level}, KELVIN ${kelvin}, LUX $lux")
        }

        val bluetoothManager: BluetoothManager = getSystemService(BluetoothManager::class.java)
        bluetoothAdapter = bluetoothManager.adapter
        blService = BluetoothService(blHandler)
        enableBluetooth()
    }

    override fun onDestroy() {
        super.onDestroy()
        // unregisterReceiver(receiver)
    }

    private fun enableBluetooth() {
        if (bluetoothAdapter == null) {
            logger.warning("Device does not support Bluetooth")
        }

        if (bluetoothAdapter?.isEnabled == false) {
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            requestBluetooth.launch(enableBtIntent)
        }

        if (bluetoothAdapter?.isDiscovering == true) {
            bluetoothAdapter?.cancelDiscovery()
        }

        /*
        bluetoothAdapter?.startDiscovery()
        registerReceiver(receiver, IntentFilter(BluetoothDevice.ACTION_FOUND))
        registerReceiver(receiver, IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED))
        */

        btWaitDialog.let {dialog ->
            dialog.setCancelable(false);
            dialog.setCanceledOnTouchOutside(false);
            dialog.show()
        }
        // TODO: Close after connect: alertDialog.dismiss()

        val device: BluetoothDevice? = bluetoothAdapter?.getRemoteDevice(HC05_BT_ADDRESS)
        if (device != null) {
            logger.info("Device: " + device.name + " : " + device.address)
            blService?.connect(device)
        }
    }

    class BlHandler(private val outerClass: WeakReference<MainActivity>) : Handler() {
        override fun handleMessage(msg: Message) {
            when (msg.what) {
                MESSAGE_WRITE -> {
                    val writeBuf = msg.obj as ByteArray
                    val writeMessage = String(writeBuf)
                }
                MESSAGE_READ -> {
                    val readBuf = msg.obj as ByteArray
                    val readMessage = String(readBuf, 0, msg.arg1)

                    val lightSwitchBtn: ToggleButton? = outerClass.get()?.findViewById(R.id.light_switch)
                    val motionDetectBtn: Switch? = outerClass.get()?.findViewById(R.id.motion_detection)
                    val levelEdit: EditText? = outerClass.get()?.findViewById(R.id.brightness)
                    val kelvinEdit: EditText? = outerClass.get()?.findViewById(R.id.color_temperature)
                    val luxEdit: EditText? = outerClass.get()?.findViewById(R.id.lux_threshold)

                    // TODO: Parse message & SET STATE

                    lightSwitchBtn?.setChecked(false)
                    motionDetectBtn?.setChecked(true)
                    levelEdit?.setText(100.toString())
                    kelvinEdit?.setText(4000.toString())
                    luxEdit?.setText(50.toString())
                }
                MESSAGE_CONNECTED -> {
                    outerClass.get()?.btWaitDialog?.dismiss()
                }
            }
        }
    }

    private var requestBluetooth = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == RESULT_OK) {
            logger.warning("Bluetooth granted")
        } else{
            logger.warning("Bluetooth denied")
        }
    }

    /*
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                BluetoothDevice.ACTION_FOUND -> {
                    val device: BluetoothDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)!!
                    bluetoothDevices.add(device)
                    logger.info("Get device: " + device.name + " : " + device.address)
                }
                BluetoothDevice.ACTION_BOND_STATE_CHANGED -> {
                    val device: BluetoothDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)!!
                    if (device.bondState == BluetoothDevice.BOND_BONDED) {
                        logger.info("Bond device: " + device.name + " : " + device.address)
                        blService?.connect(device)
                    }
                }
            }
        }
    }*/

    inner class MinMaxFilter() : InputFilter {
        // https://www.geeksforgeeks.org/how-to-set-minimum-and-maximum-input-value-in-edittext-in-android/
        private var intMin: Int = 0
        private var intMax: Int = 0

        // Initialized
        constructor(minValue: Int, maxValue: Int) : this() {
            this.intMin = minValue
            this.intMax = maxValue
        }

        override fun filter(source: CharSequence, start: Int, end: Int, dest: Spanned, dStart: Int, dEnd: Int): CharSequence? {
            try {
                val input = Integer.parseInt(dest.toString() + source.toString())
                if (isInRange(intMin, intMax, input)) {
                    return null
                }
            } catch (e: NumberFormatException) {
                e.printStackTrace()
            }
            return ""
        }

        private fun isInRange(a: Int, b: Int, c: Int): Boolean {
            return if (b > a) c in a..b else c in b..a
        }
    }

}
