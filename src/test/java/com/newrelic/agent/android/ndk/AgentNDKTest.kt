package com.newrelic.agent.android.ndk

import junit.framework.Assert
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
        Assert.assertTrue(AgentNDK.loadAgent())
        // ???
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
        // TODO
    }

    @Test
    fun dumpstack() {
        // TODO
    }
}