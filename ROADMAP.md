# Android Native Agent Roadmap

## Product Vision
The goal of the Android native agent is to provide complete visibility into the health of your app. The native agent provides metrics about the runtime health of your app and the process it runs in, with traces that show how specific requests are performing. It also provides information about the environment in which it is running, so you can identify issues with specific hosts, regions, deployments, and other facets.

New Relic is moving toward OpenTelemetry - a unified standard for service instrumentation. You will soon see an updated version of the agent that uses the OpenTelemetry SDK and auto-instrumentation as its foundation. OpenTelemetry will include a broad set of high-quality community-contributed instrumentation and a powerful vendor-neutral API for adding your own instrumentation.


## Roadmap

- **Immediate**:
    - The native agent will be integrated into our legacy Android agent, allowing native reports to be viewed directly in the customer dashboard.
    - Native reports will be **unsymbolicated** until we can seamlessly mange the symbol integration process through New Relic services. 
- **Next**:
    - Root detection, CPU profiling and native interactions.
- **Future**:
    - This section is for ideas for future work that is aligned with the product vision and possible opportunities for community contribution. It contains a list of features that anyone can implement. No guarantees can be provided on if or when these features will be completed.


#### Disclaimers
> This roadmap is subject to change at any time. Future items should not be considered commitments.
