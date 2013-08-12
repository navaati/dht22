#ifndef H_DHT22
#define H_DHT22

struct dht22_platform_data {
  unsigned gpio;
};

typedef enum {
     START,
     WARMUP,
     DATA_READ,
     DONE
} state_t;

struct dht22_priv {
  unsigned gpio;
  volatile state_t state;
  volatile ktime_t lastIntTime;
  volatile u8 data[5];
  volatile u8 bitCount;
  volatile u8 byteCount;
};

#endif /* H_DHT22 */
