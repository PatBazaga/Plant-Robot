void getDistance(){
  
  digitalWrite(TRIG ,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG , LOW);

  timeInfraredSensor = (pulseIn(ECHO , HIGH));

  dist = float(timeInfraredSensor /59);

   Serial.print("DISTANCIA:  ");
   Serial.print(dist);
   Serial.println();
}

void movementLogic(){

 int aux =  (abs(plantLightNeeds - lightR) <= LIGHT_MARGIN) && (abs(plantLightNeeds - lightL) <= LIGHT_MARGIN);
 if (aux){
  flagSleep = true;
    
  }else if (dist < DISTANCE_MIN && dist > 0){
    // Robot detects colision
     motors.setSpeed(200);
     motors.backward();
     delay(600);
     
     if(flagTurn){
       motors.setSpeedB(255);
       motors.setSpeedA(200);
       motors.forwardA();
       motors.backwardB();
     }else{
       motors.setSpeedB(200);
       motors.setSpeedA(255);
       motors.forwardB();
       motors.backwardA();
     }
    
     delay(600);
     motors.stop();
  }
  else if(abs(plantLightNeeds - lightR) > abs(plantLightNeeds - lightL)){
    // Right movement
    flagTurn = false;
    motors.setSpeedB(255);
    motors.setSpeedA(200);
    motors.forwardB();
    motors.backwardA();
    delay(650);
    motors.forward();
    delay(300);
    motors.stop();
   
  }
  else if(abs(plantLightNeeds - lightR) < abs(plantLightNeeds - lightL)){
    // Left movement
    flagTurn = true;
    motors.setSpeedA(255);
    motors.setSpeedB(200);
    motors.forwardA();
    motors.backwardB();
    delay(650);
    motors.forward();
    delay(300);
    motors.stop();
    
  }
  else {
    // Forward movement
    motors.setSpeed(200);
    motors.forward();
    delay(500);
    motors.stop();
   
  }
}

void getHumidity(){
  humidity = analogRead(HUM );
  humidity = abs (humidity/100); 
}

void waterPlant(){
  
  getHumidity();
  if ( humidity < HUMIDITY_MAX){
    flagHumidity = 0;
    digitalWrite(WATER,HIGH);
    delay(8000);
    digitalWrite(WATER,LOW); 
  }else{
    flagHumidity = 1;
  }
}


void getLight(){
  lightSensorR = analogRead(LIGHTRA);
  lightSensorL = analogRead(LIGHTLA);
  
  lightR = map(lightSensorR, 1023, 0, 0, 100); // 0 Bright light
  lightL = map(lightSensorL, 1023, 0, 0, 100); // 1023 darkness
  light = (lightR + lightL )/2; 
}