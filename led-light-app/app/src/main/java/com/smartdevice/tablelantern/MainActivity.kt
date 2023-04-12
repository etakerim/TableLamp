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
import androidx.core.text.isDigitsOnly
import java.lang.ref.WeakReference
import java.util.*
import java.util.logging.Logger


const val MESSAGE_READ: Int = 0
const val MESSAGE_WRITE: Int = 1
const val MESSAGE_CONNECTED: Int = 3
const val MESSAGE_DISCONNECT: Int = 4

// https://github.com/android/connectivity-samples/tree/master/BluetoothChat
// https://www.youtube.com/watch?v=aE8EbDmrUfQ

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

        applySettingsBtn.setOnClickListener { onApplySettings() }

        lightSwitchBtn.isEnabled = false
        lightSwitchBtn.isClickable = false
        motionDetectBtn.isEnabled = false
        motionDetectBtn.isClickable = false
        applySettingsBtn.isEnabled = false
        applySettingsBtn.isClickable = false

        // Handlers are enabled after connection

        val bluetoothManager: BluetoothManager = getSystemService(BluetoothManager::class.java)
        bluetoothAdapter = bluetoothManager.adapter
        if (bluetoothAdapter == null) {
            logger.warning("Device does not support Bluetooth")
            finish()
        }

        blService = BluetoothService(blHandler)
        if (bluetoothAdapter?.isEnabled == true) {
            lampConnect()
        } else {
            // Request enable phone bluetooth
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            requestBluetooth.launch(enableBtIntent)
        }
    }

   private fun onSwitchLight() {
       logger.info("[CMD] Switch light")
       blService?.send("SWITCH")
   }
    private fun onMotionDetectSwitch() {
        logger.info("[CMD] Change motion detection")
        blService?.send("DETECT")
    }

    private fun onApplySettings() {
        logger.info("[CMD] Apply settings")
        val levelEdit: EditText = findViewById(R.id.brightness)
        val kelvinEdit: EditText = findViewById(R.id.color_temperature)
        val luxEdit: EditText = findViewById(R.id.lux_threshold)

        val levelS = levelEdit.text.toString().trim()
        val kelvinS = kelvinEdit.text.toString().trim()
        val luxS = luxEdit.text.toString().trim()

        if (levelS.isDigitsOnly()  && kelvinS.isDigitsOnly() && luxS.isDigitsOnly()) {
            val level = levelS.toInt().coerceIn(0, 100)
            val kelvin = kelvinS.toInt().coerceIn(1000, 40000)
            val lux = luxS.toInt().coerceIn(0, 10000)

            levelEdit.setText(level.toString())
            kelvinEdit.setText(kelvin.toString())
            luxEdit.setText(lux.toString())

            val command = "LEVEL ${level}, KELVIN ${kelvin}, LUX $lux"
            blService?.send(command)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
    }

    private fun lampConnect() {
        val pairedDevices: Set<BluetoothDevice>? = bluetoothAdapter?.bondedDevices
        pairedDevices?.forEach { device ->
            logger.info("Bonded Device: " + device.name + " : " + device.address)
        }

        if (pairedDevices != null) {
            for (device in pairedDevices) {
                if (device.name == "HC-05") {
                    logger.info("HC-05 found: " + device.address)
                    blService?.connect(device)
                    break
                }
            }
        }

        btWaitDialog.let {dialog ->
            dialog.setCancelable(false);
            dialog.setCanceledOnTouchOutside(false);
            dialog.show()
        }
    }

    private var requestBluetooth = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()) { result ->
        if (result.resultCode == RESULT_OK) {
            val granted = getString(R.string.bt_granted)
            Toast.makeText(this, granted, Toast.LENGTH_SHORT).show()
            lampConnect()
        } else {
            val denied = getString(R.string.bt_denied)
            Toast.makeText(this, denied, Toast.LENGTH_SHORT).show()
        }
    }

    @SuppressLint("UseSwitchCompatOrMaterialCode")
    private fun setLightState(message: String) {
        logger.info("Command: '${message}'")
        val lightSwitchBtn: ToggleButton = findViewById(R.id.light_switch)
        val motionDetectBtn: Switch = findViewById(R.id.motion_detection)
        val applySettingsBtn: Button = findViewById(R.id.apply_btn)

        val levelEdit: EditText = findViewById(R.id.brightness)
        val kelvinEdit: EditText = findViewById(R.id.color_temperature)
        val luxEdit: EditText = findViewById(R.id.lux_threshold)

        // Turn off listeners when changing settings
        lightSwitchBtn.setOnCheckedChangeListener(null)
        motionDetectBtn.setOnCheckedChangeListener(null)

        for (option in message.removePrefix("{").removeSuffix("}").split(",")) {
            val attr = option.split("/")
            if (attr.size != 2)
                continue
            when (attr[0]) {
                "S" -> { lightSwitchBtn.isChecked = (attr[1] == "1") }
                "M" -> { motionDetectBtn.isChecked = (attr[1] == "1") }
                "B" -> { levelEdit.setText(attr[1]) }
                "K" -> { kelvinEdit.setText(attr[1]) }
                "L" -> { luxEdit.setText(attr[1]) }
            }
        }

        // Enable edit
        lightSwitchBtn.isEnabled = true
        lightSwitchBtn.isClickable = true
        motionDetectBtn.isEnabled = true
        motionDetectBtn.isClickable = true
        applySettingsBtn.isEnabled = true
        applySettingsBtn.isClickable = true

        lightSwitchBtn.setOnCheckedChangeListener { _, _ -> onSwitchLight() }
        motionDetectBtn.setOnCheckedChangeListener { _, _ -> onMotionDetectSwitch() }
    }

    class BlHandler(private val outerClass: WeakReference<MainActivity>) : Handler() {
        private val logger = Logger.getLogger(this.javaClass.name)

        override fun handleMessage(msg: Message) {
            when (msg.what) {
                MESSAGE_READ -> {
                    outerClass.get()?.setLightState(msg.obj as String)
                }
                MESSAGE_CONNECTED -> {
                    logger.info("Connected")
                    outerClass.get()?.btWaitDialog?.dismiss()
                }
                MESSAGE_DISCONNECT -> {
                    logger.info("Disconnected")
                    outerClass.get()?.lampConnect()
                }
            }
        }
    }
}
