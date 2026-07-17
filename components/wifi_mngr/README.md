# wifi_mngr

Wi-Fi station manager for cozmars-head.

On startup it tries to connect to the previously configured Wi-Fi network (credentials are stored in NVS). If no credentials exist or the connection fails, it starts a Wi-Fi hotspot with a captive-portal configuration page so the network can be set up from a phone or laptop.

## Usage

```c
#include "wifi.h"

ESP_ERROR_CHECK(wifi_init());
```

The embedded provisioning page (`web/wifi_prov.html`) is served at the root of the hotspot portal.
