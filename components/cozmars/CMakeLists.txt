idf_component_register(SRCS "src/lgfx.cpp"
                            "src/codec.c"
                            "src/camera.c"
                            "src/i2c_bus.c" "src/sub_ctl.c" "src/wled.c"
                            "src/motors.c" "src/servo.c"
                    INCLUDE_DIRS "include"
                                 "../../../cozmars-sub/sub_i2c_msg"
                    REQUIRES util
                             esp_driver_i2s LovyanGFX esp_video
                             esp_cam_sensor)
