
void setup_driver() {

  Setpoint1 = 0;
  Setpoint2 = 0;
  Setpoint3 = 0;
  Setpoint4 = 0;
  PID1.SetSampleTime(CONTROL_INTERVAL);
  PID2.SetSampleTime(CONTROL_INTERVAL);
  PID3.SetSampleTime(CONTROL_INTERVAL);
  PID4.SetSampleTime(CONTROL_INTERVAL);
  DT = (double)READ_ENCODERS_INTERVAL / 1000.0;
  PID1.SetMode(AUTOMATIC);
  PID2.SetMode(AUTOMATIC);
  PID3.SetMode(AUTOMATIC);
  PID4.SetMode(AUTOMATIC);
  PID1.SetOutputLimits(-255, 255);
  PID2.SetOutputLimits(-255, 255);
  PID3.SetOutputLimits(-255, 255);
  PID4.SetOutputLimits(-255, 255);
  md12.init();
  md12.restart();
  md34.init();
  md34.restart();


  stop_motors();


}



void read_status() {

  int faults12 = md12.getFault();
  int faults34 = md34.getFault();
  int faults_bit = 0;
  if ((faults12 != 0) || (faults34 != 0)) faults_bit = 1;

  int torque12 = !md12.getTorque();
  int torque34 = !md34.getTorque();
  int torque_bit = 0;
  if ((torque12 != 0) || (torque34 != 0)) torque_bit = 1;

  int gps_fix_bit = !gps.location.isValid();
  int imu_bit = (int)imu_fault;
  status_msg.faults = 8 * imu_bit + 4 * gps_fix_bit + 2 * torque_bit + faults_bit;
  status_msg.battery_voltage = (float)analogRead(BATTERY_MONITOR_PIN) * 3.3 / 4096 * VOLTAGE_DIVIDER_RATIO;
  p_status.publish(&status_msg);

}


void  read_encoders() {
  pre_front_left_enc = front_left_enc;
  pre_front_right_enc = front_right_enc;
  pre_rear_left_enc = rear_left_enc;
  pre_rear_right_enc = rear_right_enc;

  front_right_enc = Enc1.read(); //right
  front_left_enc = Enc2.read(); //left
  rear_right_enc = Enc3.read(); //right
  rear_left_enc = Enc4.read(); //left


  front_left_spd = alpha * (double)(front_left_enc - pre_front_left_enc) / DT  + (1 - alpha) * (double)front_left_spd;
  front_right_spd = alpha * (double)(front_right_enc - pre_front_right_enc) / DT + (1 - alpha) * (double)front_right_spd;
  rear_left_spd = alpha * (double)(rear_left_enc - pre_rear_left_enc) / DT  + (1 - alpha) * (double)rear_left_spd;
  rear_right_spd = alpha * (double)(rear_right_enc - pre_rear_right_enc) / DT + (1 - alpha) * (double)rear_right_spd;

  Input1 = front_right_spd;
  Input2 = front_left_spd;

  Input3 = rear_right_spd;
  Input4 = rear_left_spd;


}

void control_loop() {


  if ( (PID1.Compute()) && (PID2.Compute()) && (PID3.Compute()) && (PID4.Compute()) ) {
    md12.setSpeeds((int)Output1, (int)Output2);
    md34.setSpeeds((int)Output3, (int)Output4);
  }

}






void set_parametersCb(const set_parameters::Request & req, set_parameters::Response & res) {

  /*
  if ((req.kp == 0) && (req.ki == 0)&&(req.kd == 0)&&(req.alpha == 0)) {
  nh.loginfo("Setting lizi ID (will only take effect after next reboot)");
    id[0]=req.control_dt;
  flash1.write(FLASH_START, id, DATA_LENGTH);

  char log_msg[30];
    sprintf(log_msg, "New id is: = %d", (int)(((uint32_t *)FLASH_START)[0]) );
    nh.loginfo(log_msg);

  }

  else {*/
  PID1.SetTunings(req.kp, req.ki, req.kd);
  PID2.SetTunings(req.kp, req.ki, req.kd);
  PID3.SetTunings(req.kp, req.ki, req.kd);
  PID4.SetTunings(req.kp, req.ki, req.kd);
  alpha = req.alpha;
  CONTROL_INTERVAL = req.control_dt;
  PID1.SetSampleTime(CONTROL_INTERVAL);
  PID2.SetSampleTime(CONTROL_INTERVAL);
  PID3.SetSampleTime(CONTROL_INTERVAL);
  PID4.SetSampleTime(CONTROL_INTERVAL);
  //DT=(double)CONTROL_INTERVAL/1000.0;
  char log_msg[30];

  nh.loginfo("Setting parameters");
  sprintf(log_msg, "kp*10000 = %d", (int)(kp * 10000));
  nh.loginfo(log_msg);


  sprintf(log_msg, "ki*10000 = %d", (int)(ki * 10000));
  nh.loginfo(log_msg);


  sprintf(log_msg, "kd*10000 = %d", (int)(kd * 10000));
  nh.loginfo(log_msg);


  sprintf(log_msg, "alpha*10000 = %d", (int)(alpha * 10000));
  nh.loginfo(log_msg);

  sprintf(log_msg, "Control loop dt = %d", CONTROL_INTERVAL);
  nh.loginfo(log_msg);

  // }
}

void reset_encCb(const Empty::Request & req, Empty::Response & res) {
  Enc1.write(0);
  Enc2.write(0);
  Enc3.write(0);
  Enc4.write(0);

  front_left_enc = 0;
  front_right_enc = 0;
  front_left_spd = 0;
  front_right_spd = 0;
  rear_left_enc = 0;
  rear_right_enc = 0;
  rear_left_spd = 0;
  rear_right_spd = 0;
  nh.loginfo("Reset encoders");

}

void commandCb( const lizi::lizi_command& msg) {
  if (wd_on) {
    wd_on = false;
    md12.setTorque(true);
    md34.setTorque(true);
  }
  wd_t = millis();

  Setpoint2 = -msg.left_wheel;
  Setpoint1 = msg.right_wheel;
  Setpoint4 = -msg.left_wheel;
  Setpoint3 = msg.right_wheel;

  if (Setpoint1 > MAX_TICKS_PER_S) {
    Setpoint1 = MAX_TICKS_PER_S;
    Setpoint3 = MAX_TICKS_PER_S;
  }
  else if (Setpoint1 < -MAX_TICKS_PER_S) {
    Setpoint1 = -MAX_TICKS_PER_S;
    Setpoint3 = -MAX_TICKS_PER_S;
  }
  if (Setpoint2 > MAX_TICKS_PER_S) {
    Setpoint2 = MAX_TICKS_PER_S;
    Setpoint4 = MAX_TICKS_PER_S;
  }
  else if (Setpoint2 < -MAX_TICKS_PER_S) {
    Setpoint2 = -MAX_TICKS_PER_S;
    Setpoint4 = -MAX_TICKS_PER_S;
  }

}

void stop_motors( ) {


  Setpoint1 = 0;
  Setpoint2 = 0;
    Setpoint3 = 0;
  Setpoint4 = 0;
  md12.setSpeeds(0, 0); //+-255
  md12.setTorque(false);
   md34.setSpeeds(0, 0); //+-255
  md34.setTorque(false);
}

