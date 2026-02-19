    
    
    // These sketches are tested with 2.0.4 ESP32 Arduino Core
    // https://github.com/espressif/arduino-esp32/releases/tag/2.0.4
    
    /* Includes ---------------------------------------------------------------- */
    
    #include <Wire.h>
    #include <DHT.h>
    #include "SparkFun_SGP30_Arduino_Library.h"
    
    #include <agrisafe_inferencing.h>
    #include "edge-impulse-sdk/dsp/image/image.hpp"
    
    #include "esp_camera.h"
    #include <WiFi.h>
    #include <esp_now.h>
    
    
    /* Constant defines -------------------------------------------------------- */
    #define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
    #define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
    #define EI_CAMERA_FRAME_BYTE_SIZE                 3
    #define SDA_PIN 15
    #define SCL_PIN 14
    #define DHTPIN  13
    #define DHTTYPE DHT11
    
    DHT dht(DHTPIN, DHTTYPE);
    SGP30 mySensor;
    
    
    uint8_t receiverMAC[] = {0x88,0x13,0xBF,0x0B,0x9F,0xE0};  // change to your receiver MAC
    
    typedef struct {
      float temp;
      float hum;
      uint16_t co2;
      uint16_t tvoc;
      char label[20];
    }
    
    Packet;
    Packet pkt;
    
    /************* TIMERS *************/
    unsigned long sensorTimer = 0;
    unsigned long camTimer = 0;
    
    /* Private variables ------------------------------------------------------- */
    static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
    static bool is_initialised = false;
    uint8_t *snapshot_buf; //points to the output of the capture
    
    static camera_config_t camera_config = {
        .pin_pwdn = 32, 
        .pin_reset = -1,
        .pin_xclk = 0, 
        .pin_sscb_sda = 26,
        .pin_sscb_scl = 27,
          .pin_d7 = 35, 
          .pin_d6 = 34,
          .pin_d5 = 39, 
          .pin_d4 = 36,
          .pin_d3 = 21,
          .pin_d2 = 19,
          .pin_d1 = 18,
          .pin_d0 = 5,
          .pin_vsync = 25,
          .pin_href = 23,
          .pin_pclk = 22,
    
    
        //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
    
        .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    
        .jpeg_quality = 12, //0-63 lower number means higher quality
        .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };
    
    /* Function definitions ------------------------------------------------------- */
    bool ei_camera_init(void);
    void ei_camera_deinit(void);
    bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) ;
    
    /**
    * @brief      Arduino setup function
    */
    
    TaskHandle_t Task1;
    TaskHandle_t Task2;
    
    
    void setup()
    {
        // put your setup code here, to run once:
        Serial.begin(115200);
        delay(2000);
        //comment out the below line to start inference immediately after upload
        WiFi.mode(WIFI_STA);
      Wire.begin(SDA_PIN, SCL_PIN);
      dht.begin(); 
    
      if (!mySensor.begin()) while(1);
      mySensor.initAirQuality();
      
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP NOW init failed");
        return;
    }
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    esp_now_add_peer(&peerInfo);
    
    
        
        while (!Serial);
        Serial.println("Edge Impulse Inferencing Demo");
        if (ei_camera_init() == false) {
            ei_printf("Failed to initialize Camera!\r\n");
        }
        else {
            ei_printf("Camera initialized\r\n");
        }
    
        ei_printf("\nStarting continious inference in 2 seconds...\n");
        ei_sleep(2000);
    
    xTaskCreatePinnedToCore(Task1code, "Task1", 4096, NULL, 1, NULL,  1); 
      delay(1000); 
    xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, NULL,  0); 
      delay(1000); 
    }
    
    
    void Task1code( void * pvParameters ){
      Serial.print("Task1 running on core ");
      Serial.println(xPortGetCoreID());
    
      for(;;){
    
        if (millis() - sensorTimer > 1000) {
    
        sensorTimer = millis();
    
        mySensor.measureAirQuality();
    
        pkt.temp = dht.readTemperature();
        pkt.hum  = dht.readHumidity();
        pkt.co2  = mySensor.CO2;
        pkt.tvoc = mySensor.TVOC;
    
        esp_now_send(receiverMAC, (uint8_t*)&pkt, sizeof(pkt));
    
        Serial.printf("T:%.1f H:%.1f CO2:%d TVOC:%d \n",
                      pkt.temp,pkt.hum,pkt.co2,pkt.tvoc);
    
      } 
    }
    
    }
    
    void Task2code( void * pvParameters ){
      Serial.print("Task2 running on core ");
      Serial.println(xPortGetCoreID());
      for(;;){
    
      detect();
      }
    }
    
    
    
    void loop(){
    
    }
    
    
    void detect(){
    
        // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
        if (ei_sleep(5) != EI_IMPULSE_OK) {
            return;
        }
    
        snapshot_buf = (uint8_t*)malloc(320*240*3);
    
        // check if allocation was successful
        if(snapshot_buf == nullptr) {
            ei_printf("ERR: Failed to allocate snapshot buffer!\n");
            return;
        }
    
        ei::signal_t signal;
        signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
        signal.get_data = &ei_camera_get_data;
    
        if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
            ei_printf("Failed to capture image\r\n");
            free(snapshot_buf);
            return;
        }
    
        // Run the classifier
        ei_impulse_result_t result = { 0 };
    
        EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
        if (err != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", err);
            return;
        }
    
        /* print the predictions
        ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                    result.timing.dsp, result.timing.classification, result.timing.anomaly);*/
    
    #if EI_CLASSIFIER_OBJECT_DETECTION == 1
       
        //ei_printf("Object detection bounding boxes:\r\n");
        for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
            ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
            if (bb.value == 0) {
                continue;
            }
          
    
                    
                    Serial.print("label   ");
                    Serial.println(bb.label);
                 strcpy(pkt.label, bb.label);
       esp_now_send(receiverMAC, (uint8_t*)&pkt, sizeof(pkt));
    
    
        }
    
        // Print the prediction results (classification)
    #else
    
        ei_printf("Predictions:\r\n");
        for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
            ei_printf("%.5f\r\n", result.classification[i].value);
        }
    #endif
    
        // Print anomaly result (if it exists)
    #if EI_CLASSIFIER_HAS_ANOMALY
        ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
    #endif
    
    #if EI_CLASSIFIER_HAS_VISUAL_ANOMALY
        ei_printf("Visual anomalies:\r\n");
        for (uint32_t i = 0; i < result.visual_ad_count; i++) {
            ei_impulse_result_bounding_box_t bb = result.visual_ad_grid_cells[i];
            if (bb.value == 0) {
                continue;
            }
            
       
                 
        }
    #endif
    
    
        free(snapshot_buf);
    
    
    
    
      
      
      }
    
    bool ei_camera_init(void) {
    
        if (is_initialised) return true;
    
    
        //initialize the camera
        esp_err_t err = esp_camera_init(&camera_config);
        if (err != ESP_OK) {
          Serial.printf("Camera init failed with error 0x%x\n", err);
          return false;
        }
    
        sensor_t * s = esp_camera_sensor_get();
        // initial sensors are flipped vertically and colors are a bit saturated
        if (s->id.PID == OV3660_PID) {
          s->set_vflip(s, 1); // flip it back
          s->set_brightness(s, 1); // up the brightness just a bit
          s->set_saturation(s, 0); // lower the saturation
        }
    
    
        is_initialised = true;
        return true;
    }
    
    bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
        bool do_resize = false;
    
        if (!is_initialised) {
            ei_printf("ERR: Camera is not initialized\r\n");
            return false;
        }
    
        camera_fb_t *fb = esp_camera_fb_get();
    
        if (!fb) {
            ei_printf("Camera capture failed\n");
            return false;
        }
    
      bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    
       esp_camera_fb_return(fb);
    
       if(!converted){
           ei_printf("Conversion failed\n");
           return false;
       }
    
    
    
    
    
        if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
            || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
            do_resize = true;
        }
    
        if (do_resize) {
            ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
        }
    
    
        return true;
    }
    
    static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
    {
        // we already have a RGB888 buffer, so recalculate offset into pixel index
        size_t pixel_ix = offset * 3;
        size_t pixels_left = length;
        size_t out_ptr_ix = 0;
    
        while (pixels_left != 0) {
            // Swap BGR to RGB here
            // due to https://github.com/espressif/esp32-camera/issues/379
            out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
    
            // go to the next pixel
            out_ptr_ix++;
            pixel_ix+=3;
            pixels_left--;
        }
        // and done!
        return 0;
    }
    
    #if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
    #error "Invalid model for current sensor"
    #endif
