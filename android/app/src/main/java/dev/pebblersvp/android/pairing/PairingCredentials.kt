package dev.pebblersvp.android.pairing

import android.net.Uri
import org.json.JSONObject

data class PairingCredentials(
    val serverUrl: String,
    val deviceToken: String,
) {
    companion object {
        private val TokenPattern = Regex("^prsp_[0-9a-f-]{36}\\.[A-Za-z0-9_-]{43}$")

        fun parse(raw: String): PairingCredentials {
            val json = JSONObject(raw.trim())
            val serverUrl = normalizeServerUrl(json.getString("server_url"))
            val token = json.getString("device_token").trim()
            require(TokenPattern.matches(token)) { "Invalid device token" }
            return PairingCredentials(serverUrl, token)
        }

        private fun normalizeServerUrl(value: String): String {
            val uri = Uri.parse(value.trim())
            val host = uri.host?.lowercase() ?: error("Server URL requires a host")
            val localDevelopment = host == "localhost" || host == "127.0.0.1" || host == "10.0.2.2"
            require(uri.scheme == "https" || (uri.scheme == "http" && localDevelopment)) {
                "Server URL must use HTTPS"
            }
            require(uri.query == null && uri.fragment == null && uri.userInfo == null) {
                "Server URL must not contain credentials, query, or fragment"
            }
            val path = uri.path.orEmpty().trimEnd('/')
            require(path.isEmpty()) { "Server URL must not contain a path" }
            return value.trim().trimEnd('/')
        }
    }
}
