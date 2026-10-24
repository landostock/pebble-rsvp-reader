plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "dev.pebblersvp.android"
    compileSdk = 37

    defaultConfig {
        applicationId = "dev.pebblersvp.android"
        minSdk = 24
        targetSdk = 37
        versionCode = 6
        versionName = "0.3.3"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
    implementation(libs.kotlinx.coroutines.android)
    implementation(libs.pebblekit)
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.json:json:20240303")
}
