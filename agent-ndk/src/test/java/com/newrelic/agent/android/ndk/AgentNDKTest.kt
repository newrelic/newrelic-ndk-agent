package com.newrelic.agent.android.ndk

import org.junit.Assert
import org.junit.Before
import org.junit.Test

class AgentNDKTest {

    private var agentNdk: AgentNDK? = null

    companion object {
        init {
            AgentNDK.loadAgent()
        }
    }

    @Before
    fun setUp() {
        agentNdk = AgentNDK.getInstance()
        /*
        agentNdk = mock() {
            on { initialize() } doReturn true
            on { shutdown() }
        }
        */
        Assert.assertNotNull(agentNdk)
    }

    @Test
    fun loadLib() {
        // FIXME UnsatisfiedLinkError
        Assert.assertFalse("FIXME", AgentNDK.loadAgent())
    }

    @Test
    fun initialize() {
        // FIXME UnsatisfiedLinkError
        // Assert.assertTrue(agentNdk?.initialize()!!)
    }

    @Test
    fun shutdown() {
        // FIXME UnsatisfiedLinkError
        // agentNdk?.shutdown()
    }

    @Test
    fun getInstance() {
        Assert.assertNotNull(AgentNDK.getInstance())
    }

    @Test
    fun crashNow() {
        // FIXME UnsatisfiedLinkError
        // TODO
    }

    @Test
    fun dumpstack() {
        // FIXME UnsatisfiedLinkError
        // TODO
    }

    @Test
    fun cpuSample() {
        // FIXME UnsatisfiedLinkError
        // TODO
    }

}