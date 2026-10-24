package dev.pebblersvp.android.pairing

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Base64
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

class PairingStore(context: Context) {
    private val preferences = context.getSharedPreferences(PreferencesName, Context.MODE_PRIVATE)

    fun save(credentials: PairingCredentials) {
        val cipher = Cipher.getInstance(CipherTransformation)
        cipher.init(Cipher.ENCRYPT_MODE, getOrCreateKey())
        cipher.updateAAD(credentials.serverUrl.toByteArray(Charsets.UTF_8))
        val ciphertext = cipher.doFinal(credentials.deviceToken.toByteArray(Charsets.UTF_8))
        preferences.edit()
            .putString(ServerUrlKey, credentials.serverUrl)
            .putString(TokenIvKey, Base64.encodeToString(cipher.iv, Base64.NO_WRAP))
            .putString(TokenCiphertextKey, Base64.encodeToString(ciphertext, Base64.NO_WRAP))
            .apply()
    }

    fun load(): PairingCredentials? {
        val serverUrl = preferences.getString(ServerUrlKey, null) ?: return null
        val iv = preferences.getString(TokenIvKey, null) ?: return null
        val ciphertext = preferences.getString(TokenCiphertextKey, null) ?: return null
        val cipher = Cipher.getInstance(CipherTransformation)
        cipher.init(
            Cipher.DECRYPT_MODE,
            getOrCreateKey(),
            GCMParameterSpec(128, Base64.decode(iv, Base64.NO_WRAP)),
        )
        cipher.updateAAD(serverUrl.toByteArray(Charsets.UTF_8))
        val token = cipher.doFinal(Base64.decode(ciphertext, Base64.NO_WRAP)).toString(Charsets.UTF_8)
        return PairingCredentials(serverUrl, token)
    }

    fun clear() {
        preferences.edit().clear().apply()
    }

    private fun getOrCreateKey(): SecretKey {
        val keyStore = KeyStore.getInstance(AndroidKeyStore).apply { load(null) }
        (keyStore.getKey(KeyAlias, null) as? SecretKey)?.let { return it }
        return KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, AndroidKeyStore).run {
            init(
                KeyGenParameterSpec.Builder(
                    KeyAlias,
                    KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT,
                )
                    .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                    .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                    .build(),
            )
            generateKey()
        }
    }

    private companion object {
        const val AndroidKeyStore = "AndroidKeyStore"
        const val CipherTransformation = "AES/GCM/NoPadding"
        const val KeyAlias = "pebble-rsvp-device-token"
        const val PreferencesName = "pairing"
        const val ServerUrlKey = "server_url"
        const val TokenIvKey = "token_iv"
        const val TokenCiphertextKey = "token_ciphertext"
    }
}
