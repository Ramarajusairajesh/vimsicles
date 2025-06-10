package com.example.vimsicles

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

class SendActivity : ComponentActivity() {
    private var selectedUris = mutableStateListOf<Uri>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Handle shared files
        handleIntent(intent)
        
        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    SendScreen(
                        selectedUris = selectedUris,
                        onSelectFiles = {
                            // If files were shared, we don't need to show the file picker
                            if (selectedUris.isEmpty()) {
                                selectFiles()
                            }
                        },
                        onSendFiles = { ip, port ->
                            startFileTransfer(ip, port)
                        }
                    )
                }
            }
        }
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        handleIntent(intent)
    }

    private fun handleIntent(intent: Intent?) {
        when (intent?.action) {
            Intent.ACTION_SEND -> {
                intent.getParcelableExtra<Uri>(Intent.EXTRA_STREAM)?.let { uri ->
                    selectedUris.clear()
                    selectedUris.add(uri)
                }
            }
            Intent.ACTION_SEND_MULTIPLE -> {
                intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM)?.let { uris ->
                    selectedUris.clear()
                    selectedUris.addAll(uris)
                }
            }
        }
    }

    private fun selectFiles() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            type = "*/*"
        }
        startActivityForResult(intent, PICK_FILES_REQUEST_CODE)
    }

    private fun startFileTransfer(ip: String, port: Int) {
        if (selectedUris.isEmpty()) {
            Toast.makeText(this, "Please select files first", Toast.LENGTH_SHORT).show()
            return
        }

        val intent = Intent(this, FileTransferService::class.java).apply {
            action = "START_TRANSFER"
            putExtra("ip", ip)
            putExtra("port", port)
            putParcelableArrayListExtra("uris", ArrayList(selectedUris))
        }
        startService(intent)
        finish()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PICK_FILES_REQUEST_CODE && resultCode == RESULT_OK) {
            data?.let { intent ->
                if (intent.clipData != null) {
                    // Multiple files selected
                    val clipData = intent.clipData!!
                    selectedUris.clear()
                    for (i in 0 until clipData.itemCount) {
                        clipData.getItemAt(i).uri?.let { uri ->
                            selectedUris.add(uri)
                        }
                    }
                } else {
                    // Single file selected
                    intent.data?.let { uri ->
                        selectedUris.clear()
                        selectedUris.add(uri)
                    }
                }
            }
        }
    }

    companion object {
        private const val PICK_FILES_REQUEST_CODE = 1
    }
}

@Composable
fun SendScreen(
    selectedUris: List<Uri>,
    onSelectFiles: () -> Unit,
    onSendFiles: (String, Int) -> Unit
) {
    var ip by remember { mutableStateOf("") }
    var port by remember { mutableStateOf("8080") }
    val context = LocalContext.current

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Text(
            text = "Send Files",
            style = MaterialTheme.typography.headlineMedium
        )

        OutlinedTextField(
            value = ip,
            onValueChange = { ip = it },
            label = { Text("IP Address") },
            modifier = Modifier.fillMaxWidth()
        )

        OutlinedTextField(
            value = port,
            onValueChange = { port = it },
            label = { Text("Port") },
            modifier = Modifier.fillMaxWidth()
        )

        if (selectedUris.isEmpty()) {
            Button(
                onClick = onSelectFiles,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Select Files")
            }
        } else {
            Text(
                text = "Selected ${selectedUris.size} file(s)",
                style = MaterialTheme.typography.bodyLarge
            )
        }

        Button(
            onClick = {
                try {
                    onSendFiles(ip, port.toInt())
                } catch (e: NumberFormatException) {
                    Toast.makeText(context, "Invalid port number", Toast.LENGTH_SHORT).show()
                }
            },
            modifier = Modifier.fillMaxWidth(),
            enabled = ip.isNotBlank() && selectedUris.isNotEmpty()
        ) {
            Text("Send Files")
        }
    }
} 