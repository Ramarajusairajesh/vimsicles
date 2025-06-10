package com.example.vimsicles

import android.app.*
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import kotlinx.coroutines.*
import java.io.*
import java.net.ServerSocket
import java.net.Socket
import java.security.MessageDigest
import java.util.concurrent.atomic.AtomicBoolean

class FileReceiveService : Service() {
    private val TAG = "FileReceiveService"
    private val CHANNEL_ID = "FileReceiveChannel"
    private val NOTIFICATION_ID = 2
    private var serverJob: Job? = null
    private var isRunning = AtomicBoolean(false)
    private var pendingConnection: Socket? = null
    private var pendingConnectionJob: Job? = null
    private var currentProgress = 0
    private var currentFileSize = 0L

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            "START_SERVER" -> {
                val port = intent.getIntExtra("port", 8080)
                startServer(port)
            }
            "ACCEPT_CONNECTION" -> {
                val ip = intent.getStringExtra("ip")
                if (ip != null) {
                    acceptConnection(ip)
                }
            }
            "REJECT_CONNECTION" -> {
                val ip = intent.getStringExtra("ip")
                if (ip != null) {
                    rejectConnection(ip)
                }
            }
            "STOP_SERVER" -> {
                stopServer()
            }
            "UPDATE_PROGRESS" -> {
                val progress = intent.getIntExtra("progress", 0)
                updateProgress(progress)
            }
        }
        return START_STICKY
    }

    private fun startServer(port: Int) {
        if (isRunning.get()) return

        serverJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                val serverSocket = ServerSocket(port)
                isRunning.set(true)
                updateNotification("Server running on port $port", 0)

                while (isRunning.get()) {
                    try {
                        val clientSocket = serverSocket.accept()
                        val clientIp = clientSocket.inetAddress.hostAddress
                        
                        // Send connection request to activity
                        val intent = Intent(this@FileReceiveService, ReceiveActivity::class.java).apply {
                            flags = Intent.FLAG_ACTIVITY_NEW_TASK
                            putExtra("pending_connection", clientIp)
                        }
                        startActivity(intent)

                        // Store pending connection
                        pendingConnection = clientSocket
                        pendingConnectionJob = CoroutineScope(Dispatchers.IO).launch {
                            try {
                                // Wait for user response
                                delay(30000) // 30 second timeout
                                if (pendingConnection == clientSocket) {
                                    // If still pending after timeout, reject
                                    rejectConnection(clientIp)
                                }
                            } catch (e: Exception) {
                                Log.e(TAG, "Error in pending connection: ${e.message}")
                            }
                        }
                    } catch (e: Exception) {
                        if (isRunning.get()) {
                            Log.e(TAG, "Error accepting connection: ${e.message}")
                        }
                    }
                }
                serverSocket.close()
            } catch (e: Exception) {
                Log.e(TAG, "Error starting server: ${e.message}")
                stopSelf()
            }
        }
    }

    private fun acceptConnection(ip: String) {
        pendingConnectionJob?.cancel()
        val socket = pendingConnection
        pendingConnection = null

        if (socket != null && socket.inetAddress.hostAddress == ip) {
            CoroutineScope(Dispatchers.IO).launch {
                try {
                    // Send acceptance message
                    val writer = PrintWriter(socket.getOutputStream(), true)
                    writer.println("HELLO")

                    // Start receiving files
                    receiveFiles(socket)
                } catch (e: Exception) {
                    Log.e(TAG, "Error accepting connection: ${e.message}")
                    socket.close()
                }
            }
        }
    }

    private fun rejectConnection(ip: String) {
        pendingConnectionJob?.cancel()
        val socket = pendingConnection
        pendingConnection = null

        if (socket != null && socket.inetAddress.hostAddress == ip) {
            try {
                socket.close()
            } catch (e: Exception) {
                Log.e(TAG, "Error rejecting connection: ${e.message}")
            }
        }
    }

    private fun stopServer() {
        isRunning.set(false)
        serverJob?.cancel()
        pendingConnectionJob?.cancel()
        pendingConnection?.close()
        pendingConnection = null
        stopForeground(true)
        stopSelf()
    }

    private fun updateProgress(progress: Int) {
        currentProgress = progress
        updateNotification("Receiving: $progress%", progress)
    }

    private suspend fun receiveFiles(socket: Socket) {
        try {
            val reader = BufferedReader(InputStreamReader(socket.getInputStream()))
            val writer = PrintWriter(socket.getOutputStream(), true)

            // Read archive name and MD5
            val archiveName = reader.readLine()
            val expectedMd5 = reader.readLine()

            if (archiveName == null || expectedMd5 == null) {
                throw IOException("Invalid archive information")
            }

            // Create vimsicles directory if it doesn't exist
            val vimsiclesDir = File(getExternalFilesDir(null), "vimsicles")
            if (!vimsiclesDir.exists()) {
                vimsiclesDir.mkdirs()
            }

            // Create temporary file for the archive
            val tempFile = File(vimsiclesDir, "temp_$archiveName")
            val fileOutputStream = FileOutputStream(tempFile)
            val buffer = ByteArray(8192)
            var bytesRead: Int
            var totalBytesRead = 0L

            // Read file size from sender (if available)
            val sizeLine = reader.readLine()
            currentFileSize = sizeLine?.toLongOrNull() ?: 0L

            updateNotification("Preparing to receive...", 0)

            // Read file data
            val inputStream = socket.getInputStream()
            while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                fileOutputStream.write(buffer, 0, bytesRead)
                totalBytesRead += bytesRead
                if (currentFileSize > 0) {
                    val progress = (totalBytesRead * 100 / currentFileSize).toInt()
                    updateProgress(progress)
                } else {
                    updateNotification("Receiving: ${formatFileSize(totalBytesRead)}", 0)
                }
            }

            fileOutputStream.close()

            // Calculate MD5 hash
            val md5 = calculateMD5(tempFile)
            if (md5 != expectedMd5) {
                tempFile.delete()
                throw IOException("MD5 hash mismatch. File may be corrupted.")
            }

            // Move file to final location
            val finalFile = File(vimsiclesDir, archiveName)
            tempFile.renameTo(finalFile)

            // Extract the archive
            updateNotification("Extracting files...", 0)
            val process = Runtime.getRuntime().exec("cd ${vimsiclesDir.absolutePath} && gunzip -c $archiveName | tar xf -")
            process.waitFor()

            if (process.exitValue() == 0) {
                // Delete the archive after successful extraction
                finalFile.delete()
                updateNotification("Files received and extracted successfully", 100)
            } else {
                throw IOException("Failed to extract archive")
            }

        } catch (e: Exception) {
            Log.e(TAG, "Error receiving files: ${e.message}")
            updateNotification("Error: ${e.message}", 0)
        } finally {
            try {
                socket.close()
            } catch (e: Exception) {
                Log.e(TAG, "Error closing socket: ${e.message}")
            }
        }
    }

    private fun calculateMD5(file: File): String {
        val md = MessageDigest.getInstance("MD5")
        val buffer = ByteArray(8192)
        var bytesRead: Int

        FileInputStream(file).use { inputStream ->
            while (inputStream.read(buffer).also { bytesRead = it } != -1) {
                md.update(buffer, 0, bytesRead)
            }
        }

        return md.digest().joinToString("") { "%02x".format(it) }
    }

    private fun formatFileSize(size: Long): String {
        val units = arrayOf("B", "KB", "MB", "GB")
        var value = size.toDouble()
        var unitIndex = 0

        while (value >= 1024 && unitIndex < units.size - 1) {
            value /= 1024
            unitIndex++
        }

        return "%.1f %s".format(value, units[unitIndex])
    }

    private fun updateNotification(message: String, progress: Int) {
        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("File Receive Service")
            .setContentText(message)
            .setSmallIcon(android.R.drawable.sym_def_app_icon)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setProgress(100, progress, false)
            .setOngoing(true)
            .build()

        startForeground(NOTIFICATION_ID, notification)
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "File Receive Service",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Shows file receive progress"
            }

            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()
        stopServer()
    }
} 