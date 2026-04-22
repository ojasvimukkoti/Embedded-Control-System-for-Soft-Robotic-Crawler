/*
Code for testing conenction between ESP32 feather and mini air pump.

Re-inflate + evacuate (air flow out of kresling, thus contracting it)

Need to figure out how to reverse pump logic so it "sucks" out the air 
 - to remove air from kresling we need the air valve to be open to release air back into the kresling


*/

//indicating pin connected to MOSFET driver (transistor)
const int pumpPin1 = 26;
const int valvePin1 = 25;
const int pumpPin2 = 33;
const int valvePin2 = 32;

void crawl_sequence(int steps){
  // int steps = steps; // number of steps for crawler to moves
  // int stepTime_ms = 200 // step waiting time
  for (int i = 0; i<steps; i++){
    //air leaves Kresling 1
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, HIGH);

    delay(3200);

    //kresling 1 put air back in
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, LOW);

    delay(1000); //small pause (optional)

    //air leaves Kresling 2
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, HIGH);

    delay(5200);

    //kresling 2 put air back in
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, LOW);

    delay(1000); //small pause (optional

  }
}


void command_callback(const void * msgin)
{
  const std_msgs__msg__String * msg = (const std_msgs__msg__String *)msgin;
  String command = String(msg->data.data);

  command.trim();
  command.toUpperCase();

  Serial.print("Received: ");
  Serial.println(command);

  if (command == "I"){
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, LOW);
  }
  else if (command == "A"){
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, LOW);
  }
  else if (command == "D"){
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, HIGH);
  }
  else if (command == "E"){
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, HIGH);
  }
  else if (command.startsWith("C")) {
    int steps = command.substring(1).toInt(); // e.g. "C3"
    crawl_sequence(steps);
  }
  else if (command == "O"){
    digitalWrite(pumpPin1, LOW);
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin2, LOW);
    digitalWrite(valvePin2, LOW);
  }
}

void setup() {

  Serial.begin(115200);
  while(!Serial) {delay(1);}

  Serial.println("Testing Kresling Control: ");
  Serial.println("D: Deflate Kresling 1");
  Serial.println("I: Release Kresling 1");

  Serial.println("E: Deflate Kresling 2");
  Serial.println("A: Release Kresling 2");

  Serial.println("C: Crawling Sequence (both Kreslings)"); 

  Serial.println("O: All off Kreslings");

  pinMode(pumpPin1, OUTPUT);
  pinMode(valvePin1, OUTPUT);

  digitalWrite(pumpPin1, LOW); //pump off
  digitalWrite(valvePin1, LOW); //valve closed

  pinMode(pumpPin2, OUTPUT);
  pinMode(valvePin2, OUTPUT);

  digitalWrite(pumpPin2, LOW); //pump off
  digitalWrite(valvePin2, LOW); //valve closed

}


void loop() {
  if (Serial.available() > 0){
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();

    Serial.print("Processing command: ");
    Serial.println(command);

    if (command =="I"){ //air goes back into Kresling
      //inflate Kresling 1
      digitalWrite(valvePin1, LOW); //close valve
      digitalWrite(pumpPin1, LOW); //pump ON
      Serial.println("Releasing Kresling");
    }

    else if (command == "A"){ 
      //inflate Kresling 2
      digitalWrite(valvePin2, LOW);
      digitalWrite(pumpPin2, LOW); 
      Serial.println("Releasing Kresling");
    
    }
    else if (command == "D"){ // releasing air from kresling
      //deflating for Kresling 1
      digitalWrite(valvePin1, LOW); //valve 
      digitalWrite(pumpPin1, HIGH); //pump OFF
      Serial.println("Deflating Kresling");
    
    }
    else if (command == "E"){ 
      //deflating for Kresling 2
      digitalWrite(valvePin2, LOW); //valve 
      digitalWrite(pumpPin2, HIGH); //pump OFF
      Serial.println("Deflating Kresling");
    
    }
    else if (command == "C"){
      Serial.println("Starting crawling sequence");
      Serial.println("Enter number of steps for crawler: ");

      while(Serial.available() == 0){
        //waiting for input
      }

      int crawl_steps = Serial.parseInt();

      Serial.print("Crawler walking: ");
      Serial.println(crawl_steps);

      crawl_sequence(crawl_steps);
    }
    else if (command == "O"){
      //turn off all Kreslings
      digitalWrite(pumpPin1, LOW);
      digitalWrite(valvePin1, LOW);

      digitalWrite(pumpPin2, LOW);
      digitalWrite(valvePin2, LOW);
      Serial.println("All outputs OFF");
    }

  }

}
