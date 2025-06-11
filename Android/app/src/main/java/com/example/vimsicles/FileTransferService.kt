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
import java.net.Socket
import java.security.MessageDigest
import java.util.concurrent.atomic.AtomicBoolean

class FileTransferService : Service() {
    private val TAG = "FileTransferService"
    private val CHANNEL_ID = "FileTransferChannel"
    private val NOTIFICATION_ID = 1
    private var transferJob: Job? = null
    private var isRunning = AtomicBoolean(false)
    private var currentProgress = 0
    private var currentFileSize = 0L

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            "START_TRANSFER" -> {
                val ip = intent.getStringExtra("ip")
                val port = intent.getIntExtra("port", 8080)
                val uris = intent.getParcelableArrayListExtra<android.net.Uri>("uris")
                if (ip != null && uris != null) {
                    startTransfer(ip, port, uris)
                }
            }
            "STOP_TRANSFER" -> {
                stopTransfer()
            }
            "UPDATE_PROGRESS" -> {
                val progress = intent.getIntExtra("progress", 0)
                updateProgress(progress)
            }
        }
        return START_STICKY
    }

    private fun startTransfer(ip: String, port: Int, uris: ArrayList<android.net.Uri>) {
        if (isRunning.get()) return

        transferJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                val socket = Socket(ip, port)
                isRunning.set(true)

                // Send archive name and MD5 first
                val archiveName = "transfer_${System.currentTimeMillis()}.tar.gz"
                val tempFile = createTempArchive(uris, archiveName)
                val md5 = calculateMD5(tempFile)
                currentFileSize = tempFile.length()

                val writer = PrintWriter(socket.getOutputStream(), true)
                val type = if (uris.size > 1) "folder" else "file"
                writer.println("${type}|${archiveName}|${md5}")

                // Wait for HELLO response
                val reader = BufferedReader(InputStreamReader(socket.getInputStream()))
                val response = reader.readLine()

                if (response == "HELLO") {
                    // Start sending the file
                    val fileInputStream = FileInputStream(tempFile)
                    val outputStream = socket.getOutputStream()
                    val buffer = ByteArray(8192)
                    var bytesRead: Int
                    var totalBytesRead = 0L

                    updateNotification("Preparing to send...", 0)

                    while (fileInputStream.read(buffer).also { bytesRead = it } != -1) {
                        outputStream.write(buffer, 0, bytesRead)
                        totalBytesRead += bytesRead
                        val progress = (totalBytesRead * 100 / currentFileSize).toInt()
                        updateProgress(progress)
                    }

                    fileInputStream.close()
                    outputStream.flush()
                    tempFile.delete()
                    updateNotification("Transfer completed", 100)
                } else {
                    throw IOException("Connection rejected by receiver")
                }

                socket.close()
            } catch (e: Exception) {
                Log.e(TAG, "Error during transfer: ${e.message}")
                updateNotification("Error: ${e.message}", 0)
            } finally {
                isRunning.set(false)
                stopSelf()
            }
        }
    }

    private fun stopTransfer() {
        isRunning.set(false)
        transferJob?.cancel()
        stopForeground(true)
        stopSelf()
    }

    private fun updateProgress(progress: Int) {
        currentProgress = progress
        updateNotification("Sending: $progress%", progress)
    }

    private suspend fun createTempArchive(uris: ArrayList<android.net.Uri>, archiveName: String): File {
        val tempDir = File(cacheDir, "temp_transfer")
        if (!tempDir.exists()) {
            tempDir.mkdirs()
        }

        // Copy files to temp directory
        uris.forEach { uri ->
            val inputStream = contentResolver.openInputStream(uri)
            val fileName = getFileName(uri)
            val file = File(tempDir, fileName)
            
            inputStream?.use { input ->
                FileOutputStream(file).use { output ->
                    input.copyTo(output)
                }
            }
        }

        // Create tar.gz archive
        val archiveFile = File(cacheDir, archiveName)
        val process = Runtime.getRuntime().exec("cd ${tempDir.absolutePath} && tar czf ${archiveFile.absolutePath} .")
        process.waitFor()

        // Clean up temp directory
        tempDir.deleteRecursively()

        return archiveFile
    }

    private fun getFileName(uri: android.net.Uri): String {
        val cursor = contentResolver.query(uri, null, null, null, null)
        val nameIndex = cursor?.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
        val name = if (nameIndex != null && nameIndex != -1) {
            cursor.moveToFirst()
            cursor.getString(nameIndex)
        } else {
            uri.lastPathSegment ?: "unknown_file"
        }
        cursor?.close()
        return name
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

    private fun updateNotification(message: String, progress: Int) {
        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("File Transfer Service")
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
                "File Transfer Service",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Shows file transfer progress"
            }

            val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()
        stopTransfer()
    }
} 