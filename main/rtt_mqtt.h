#ifndef RTT_MQTT_H
#define RTT_MQTT_H

// Журнал SEGGER RTT цели без ПК: по SWD ищет блок RTT в RAM цели, читает кольцо «up 0»
// и шлёт строки по MQTT в rtt/<mac>. Режим по умолчанию ВЫКЛ (Kconfig / NVS «rtt»),
// чтобы не мешать OpenOCD: оба ходят по одному SWD.
void rtt_mqtt_start(void);

#endif
