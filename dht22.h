#ifndef H_DHT22
#define H_DHT22

struct dht22_platform_data {
  unsigned gpio;
};

typedef enum {
     READY,
     START,
     WARMUP,
     DATA_READ,
     DONE
} state_t;

struct dht22_priv {
  unsigned gpio;
  volatile state_t state;
  volatile ktime_t lastIntTime;
  volatile union {
    volatile u8 data[5];
    struct {
      u8 rh_int;
      u8 rh_dec;
      u8 t_int;
      u8 t_dec;
      u8 checksum;
    };
  };
  volatile int bitCount;
  volatile int byteCount;
};

#endif /* H_DHT22 */
