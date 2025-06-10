package com.example.vimsicles

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.core.*
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay

class ReceiveActivity : ComponentActivity() {
    private var isServerRunning by mutableStateOf(false)
    private var pendingConnection by mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    ReceiveScreen(
                        isServerRunning = isServerRunning,
                        pendingConnection = pendingConnection,
                        onStartServer = { port ->
                            startFileReceiveServer(port)
                        },
                        onAcceptConnection = { ip ->
                            acceptConnection(ip)
                        },
                        onRejectConnection = { ip ->
                            rejectConnection(ip)
                        },
                        onCancel = {
                            stopServer()
                            finish()
                        }
                    )
                }
            }
        }
    }

    private fun startFileReceiveServer(port: Int) {
        val intent = Intent(this, FileReceiveService::class.java).apply {
            action = "START_SERVER"
            putExtra("port", port)
        }
        startService(intent)
        isServerRunning = true
    }

    private fun acceptConnection(ip: String) {
        val intent = Intent(this, FileReceiveService::class.java).apply {
            action = "ACCEPT_CONNECTION"
            putExtra("ip", ip)
        }
        startService(intent)
        pendingConnection = null
    }

    private fun rejectConnection(ip: String) {
        val intent = Intent(this, FileReceiveService::class.java).apply {
            action = "REJECT_CONNECTION"
            putExtra("ip", ip)
        }
        startService(intent)
        pendingConnection = null
    }

    private fun stopServer() {
        val intent = Intent(this, FileReceiveService::class.java).apply {
            action = "STOP_SERVER"
        }
        startService(intent)
        isServerRunning = false
    }
}

@Composable
fun ReceiveScreen(
    isServerRunning: Boolean,
    pendingConnection: String?,
    onStartServer: (Int) -> Unit,
    onAcceptConnection: (String) -> Unit,
    onRejectConnection: (String) -> Unit,
    onCancel: () -> Unit
) {
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
            text = "Receive Files",
            style = MaterialTheme.typography.headlineMedium
        )

        if (!isServerRunning) {
            OutlinedTextField(
                value = port,
                onValueChange = { port = it },
                label = { Text("Port") },
                modifier = Modifier.fillMaxWidth()
            )

            Button(
                onClick = {
                    try {
                        onStartServer(port.toInt())
                    } catch (e: NumberFormatException) {
                        Toast.makeText(context, "Invalid port number", Toast.LENGTH_SHORT).show()
                    }
                },
                modifier = Modifier.fillMaxWidth(),
                enabled = port.isNotBlank()
            ) {
                Text("Start Server")
            }
        } else {
            // Show discovery animation
            DiscoveryAnimation()

            // Show pending connection dialog if any
            pendingConnection?.let { ip ->
                AlertDialog(
                    onDismissRequest = { onRejectConnection(ip) },
                    title = { Text("Incoming Connection") },
                    text = { Text("Device at $ip wants to send files. Accept?") },
                    confirmButton = {
                        TextButton(onClick = { onAcceptConnection(ip) }) {
                            Text("Yes")
                        }
                    },
                    dismissButton = {
                        TextButton(onClick = { onRejectConnection(ip) }) {
                            Text("No")
                        }
                    }
                )
            }

            // Add cancel button at the bottom
            Spacer(modifier = Modifier.weight(1f))
            Button(
                onClick = onCancel,
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.buttonColors(
                    containerColor = MaterialTheme.colorScheme.error
                )
            ) {
                Text("Cancel")
            }
        }
    }
}

@Composable
fun DiscoveryAnimation() {
    val infiniteTransition = rememberInfiniteTransition()
    val scale by infiniteTransition.animateFloat(
        initialValue = 0.5f,
        targetValue = 1f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000),
            repeatMode = RepeatMode.Reverse
        )
    )

    val alpha by infiniteTransition.animateFloat(
        initialValue = 1f,
        targetValue = 0f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000),
            repeatMode = RepeatMode.Reverse
        )
    )

    Box(
        modifier = Modifier
            .size(200.dp)
            .padding(16.dp),
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val center = Offset(size.width / 2, size.height / 2)
            val radius = size.width.coerceAtMost(size.height) / 2 * scale

            // Draw multiple circles with different opacities
            for (i in 0..2) {
                val circleScale = scale * (1f - i * 0.2f)
                val circleAlpha = alpha * (1f - i * 0.2f)
                drawCircle(
                    color = Color.Blue.copy(alpha = circleAlpha),
                    radius = radius * circleScale,
                    center = center,
                    style = Stroke(width = 4f)
                )
            }
        }
    }

    Text(
        text = "Waiting for devices...",
        style = MaterialTheme.typography.bodyLarge,
        modifier = Modifier.padding(top = 16.dp)
    )
} 