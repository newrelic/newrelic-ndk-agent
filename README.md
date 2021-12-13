[![Community Plus header](https://github.com/newrelic/opensource-website/raw/master/src/images/categories/Community_Plus.png)](https://opensource.newrelic.com/oss-category/#community-plus)
# newrelic-android-ndk

With [New Relic's Android native agent](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/), you can capture native crashes resulting from raised signals and uncaught runtime exceptions from C and C++ code used in your Android app.

During startup, the native agent installs handlers for important signals, a C++ unhandled exception handler and a monitor to detect ANR conditions.

In the event of a crash, the native agent will generally not have enough time or stability to process the report. Instead, the report data is quickly written to local storage, to be processed the next time the app is launched. It is up to the app to request reports when ready.

Android native agent releases follow [semantic versioning conventions](https://semver.org/). See [Android native agent release notes](https://docs.newrelic.com/docs/release-notes/mobile-release-notes/android-release-notes/) for full details on releases and downloads. Android native agent artifacts can also be found on [Maven Central](https://search.maven.org/search?q=com.newrelic.agent.android).

### Native Reports

The native agent will report three types of native events:

* Raised signal (crash)

* Unhandled exceptions

* Application Not responding (ANR)


## Getting started

See the [getting started guide](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/getting-started) as well as the [compatibility and requirements documentation](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/getting-started/compatibility-requirements-java-agent) for an overview of what is supported by the Android native agent.

## Usage

See the following documentation for specific use cases of the Android native agent:
- [General configuration](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/configuration)
- [Troubleshooting](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/troubleshooting)

## Installation

#### Add agent to build dependencies
To add the Android native agent to your app, you simply need to declare a dependency in your app-level Gradle build file:
```
dependencies {
    implementation 'com.newrelic.agent.android:agent-ndk:<version>'
}
```

#### Declare a report listener

The report listener is used to pass native reports from the agent to your app. Reports are represented as JSON strings. The format of the JSON report is [here](FIXME).

Implement the following interface, and pass a reference when starting the native agent:

```
class Listener : AgentNDKListener {
    /**
     * A native crash has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    override fun onNativeCrash(crashAsString: String?): Boolean {
        return false
    }

    /**
     * A native runtime exception has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    override fun onNativeException(exceptionAsString: String?): Boolean {
        return false
    }

    /**
     * ANR condition has been detected and forwarded to this method
     * @param String containing backtrace
     * @return true if data has been consumed
     */
    override fun onApplicationNotResponding(anrAsString: String?): Boolean {
        return false
    }
}
```
#### Build and start the agent
Finally, instantiate the agent in either the Application or launcher Activity class, passing the application context and a reference to your report listener implementation:
```
// construct an agent instance
var agentNDK = AgentNDK.Builder(context)
    .withReportListener(Listener())
    .build()

// when ready, start the agent
agentNDK.start()

// when finished, stop the agent
agentNDK.stop()

// Alternatively, the agent can be stopped by calling the static method
AgentNDK.shutdown()
```

The agent exists as a single instance (singleton), and any further uses of the builder will replace the instance

```
var agentNDK = AgentNDK.getInstance()

AgentNDK.Builder(context)
    .withReportListener(Listener())
    .build()

// agentNDK is now invalid

```

#### Requesting Queued Reports
In the event of a crash, the native agent will generally not have enough time or stability to process the report. Instead, the report data is quickly written to local storage, to be processed the next time the app is launched. It is up to the app to request reports when it is ready for them by calling:
```
agentNDK.flushPendingReports()
```

#### Expiring Reports
Reports that remain in the cache longer that remain in the cache longer than the configured expiration duration will be removed, but only after a final post to the listener.

```
// The expiration time is set during agent creation by adding this build method
val REPORT_TTL = TimeUnit.SECONDS.convert(3, TimeUnit.DAYS)
val agentNDK = AgentNDK.Builder(context)
    .withReportListener(Listener())
    .withExpiration(REPORT_TTL)
    .build()

// But can also be set at runtime
agent.managedContext?.expirationPeriod = REPORT_TTL
```

#### Additional Builder methods

##### AgentNDK.Builder.withBuildId(buildId: String)
##### AgentNDK.Builder.withSessionId(sessionId: String)

The agent will inject a 40-character build and session id into each generated report.
These values will be created, but can also be specified when the agent is built.

```
// the default values are UUIDs
val buildId = UUID.randomUUID().toString()
val sessionId = UUID.randomUUID().toString()

AgentNDK.Builder(context)
  .withBuildId(buildId)
  .withSessionId(sessionId)
  .build()
```

##### AgentNDK.Builder.withStorageDir(storageRootDir: File)

Sets the agent's report directory inside of the passed storage location, creating the subdirectories as needed.
```
// the default location is /data/data/<app>/cache/newrelic/reports
AgentNDK.Builder(context)
  .withStorageDir(context?.cacheDir)
```

### Receiving Queued Reports
In the event of a crash, the native agent will generally not have enough time or stability to process the report. Instead, the report data is quickly written to local storage, to be processed the next time the app is launched. It is up to the app to request reports when it is ready for them by calling:
```
agentNDK.flushPendingReports()
```

#### Expiring Reports
Reports that remain in the cache longer that remain in the cache longer than the configured expiration duration will be removed, but only after a final post to the listener.

```
// The expiration time is set during agent creation by adding this build method
val REPORT_TTL = TimeUnit.SECONDS.convert(3, TimeUnit.DAYS)
val agentNDK = AgentNDK.Builder(context)
    .withReportListener(Listener())
    .withExpiration(REPORT_TTL)
    .build()

// But can also be set at runtime
agent.managedContext?.expirationPeriod = REPORT_TTL
```

### For full details see:

- [General installation instructions](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/install-configure/)
- [Additional installation instructions (Maven, Gradle, etc)](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/additional-installation)


## Building

The Android native agent requires the following tools to build:

|Dependency|Version| |
|----------|-------|-----|
|Java| JDK 8 or higher||
|Android Gradle Plugin|4.1 or higher|AGP 7 requires JDK 11|
|Gradle|6.7.1|AGP 7 requires Gradle 7 or higher|
|NDK|21.4.7075529 or higher||
|minSDK|24||

Dependencies must to be installed and configured for your environment prior to building.

### Gradle build

The Android native agent (`agent-ndk`) requires JDK 8 or higher to build; your `JAVA_HOME` must be set to this JDK version.

To build the `agent-ndk` AAR, run the following command from the project root directory:  

```
./gradlew clean assemble
```

To build and run all checks:

```
./gradlew clean build
```

After building, Android native agent artifacts are located in `agent-ndk/build/outputs/aar`

## Android Studio setup

Android Studio must be configured with the Android Native Development Kit (NDK) installed.

## Testing

The unit tests are conventional JUnit tests. The supporting framework is the industry-standard JUnit dependency. Unit tests rely on a variety of different mock object frameworks combined with complex test initialization patterns that simulate agent initialization.

To run all unit tests from the command line:
```
./gradlew clean test
```

## Support

Should you need assistance with New Relic products, you are in good hands with several diagnostic tools and support channels.

If the issue has been confirmed as a bug or is a Feature request, please file a Github issue.

**Support Channels**

* [New Relic Documentation](https://docs.newrelic.com/docs/mobile-monitoring/new-relic-mobile-android/): Comprehensive guidance for using our platform
* [New Relic Community](https://discuss.newrelic.com/tags/android-agent): The best place to engage in troubleshooting questions
* [New Relic Technical Support](https://support.newrelic.com/) 24/7/365 ticketed support. Read more about our [Technical Support Offerings](https://docs.newrelic.com/docs/licenses/license-information/general-usage-licenses/support-plan).

## Privacy
At New Relic we take your privacy and the security of your information seriously, and are committed to protecting your information. We must emphasize the importance of not sharing personal data in public forums, and ask all users to scrub logs and diagnostic information for sensitive information, whether personal, proprietary, or otherwise.

We define _personal data_ as any information relating to an identified or identifiable individual, including for example your name, phone number, post or zip code, the sender's IP or email address.

Please review [New Relic’s General Data Privacy Notice](https://newrelic.com/termsandconditions/privacy) for more information.

## Roadmap
Our native agent [feature roadmap](/ROADMAP.md) contains more about our product vision, including our plans and rough time lines for introducing new features. Your feedback is encouraged.

## Contribute
We encourage your contributions to improve `android-agent-ndk`. Keep in mind when you submit your pull request, you'll need to sign the CLA via the click-through using CLA-Assistant. You only have to sign the CLA one time per project.

If you have any questions, or to execute our corporate CLA, required if your contribution is on behalf of a company, please drop us an email at opensource@newrelic.com.

**A note about vulnerabilities**

As noted in our [security policy](https://github.com/newrelic/android-agent-ndk/security/policy), New Relic is committed to the privacy and security of our customers and their data. We believe that providing coordinated disclosure by security researchers and engaging with the security community are important means to achieve our security goals.

If you believe you have found a security vulnerability in this project or any of New Relic's products or websites, we welcome and greatly appreciate you reporting it to New Relic through [HackerOne](https://hackerone.com/newrelic).

## License
`android-agent-ndk` is licensed under the [Apache 2.0](http://apache.org/licenses/LICENSE-2.0.txt) License.

The `android-agent-ndk` also uses source code from third-party libraries. You can find full details on which libraries are used and the terms under which they are licensed in the third-party notices document and our [Android native agent licenses public documentation](https://docs.newrelic.com/docs/licenses/product-or-service-licenses/new-relic-apm/java-agent-licenses).
