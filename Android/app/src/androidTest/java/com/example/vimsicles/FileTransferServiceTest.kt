package com.example.vimsicles

import android.content.Intent
import android.net.Uri
import android.os.IBinder
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.rule.ServiceTestRule
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.util.concurrent.TimeoutException
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.delay

@RunWith(AndroidJUnit4::class)
class FileTransferServiceTest {

    @get:Rule
    val serviceRule = ServiceTestRule()

    private lateinit var service: FileTransferService

    @Before
    fun setup() {
        val serviceIntent = Intent(ApplicationProvider.getApplicationContext(), FileTransferService::class.java)
        service = FileTransferService()
        serviceRule.startService(serviceIntent)
    }

    @Test
    fun testStartTransfer() {
        val ip = "127.0.0.1"
        val port = 8080
        val uris = ArrayList<Uri>()
        // Add test URIs here if needed

        val intent = Intent(ApplicationProvider.getApplicationContext(), FileTransferService::class.java).apply {
            action = "START_TRANSFER"
            putExtra("ip", ip)
            putExtra("port", port)
            putExtra("uris", uris)
        }

        serviceRule.startService(intent)

        // Wait for the service to complete or timeout
        try {
            serviceRule.waitForIdle(5000)
        } catch (e: TimeoutException) {
            // Handle timeout if necessary
        }
    }

    @Test
    fun testStopTransfer() {
        val intent = Intent(ApplicationProvider.getApplicationContext(), FileTransferService::class.java).apply {
            action = "STOP_TRANSFER"
        }

        serviceRule.startService(intent)

        // Wait for the service to complete or timeout
        try {
            serviceRule.waitForIdle(5000)
        } catch (e: TimeoutException) {
            // Handle timeout if necessary
        }
    }

    @Test
    fun testUpdateProgress() {
        val intent = Intent(ApplicationProvider.getApplicationContext(), FileTransferService::class.java).apply {
            action = "UPDATE_PROGRESS"
            putExtra("progress", 50)
        }

        serviceRule.startService(intent)

        // Wait for the service to complete or timeout
        try {
            serviceRule.waitForIdle(5000)
        } catch (e: TimeoutException) {
            // Handle timeout if necessary
        }
    }
} 