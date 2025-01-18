class ETSSettings {
  int ledBrightness;
  bool isDCMotor;
  int paperTime;
  int stepperCount;
  int stepperTime;

  ETSSettings({
    this.ledBrightness = 50,
    this.isDCMotor = true,
    this.paperTime = 1000,
    this.stepperCount = 100,
    this.stepperTime = 1000,
  });

  Map<String, dynamic> toJson() => {
        'LED_BRIGHT': ledBrightness,
        'PAPER_TIME': isDCMotor ? paperTime : 0,
        'STEP_COUNT': !isDCMotor ? stepperCount : 0,
        'STEP_TIME': !isDCMotor ? stepperTime : 0,
      };
}