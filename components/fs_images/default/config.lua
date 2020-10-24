require "types"

config = {}

config.nc   = -1
config.axis = {}
config.actuator = {}

-- Axis
config.axis.x = {
	class      = "XYAxis",
    enabled    = true,
    enPin      = 40,
    stepPin    = 2,
    dirPin     = 41,
    dirPinInv  = false,
    stepsPerMM = 28.57,
    minSpeed   = 1000,
    maxSpeed   = 60000,
    maxAccel   = 2000,
    maxJerk    = 30000,
    
    endStop = {
              minPin = 49,
              maxPin = 50,
			  homingOnly = false,
              homingDirection = HomingDirectionType.HomingToMin,
              homingSpeed = 1000,
              minHomingPos = 0,
              maxHomingMax = 200, 
              homingRetract = 5,
      }
}

config.axis.y = {
	class      = "XYAxis",
    enabled    = true,
    enPin      = 40,
    stepPin    = 15,
    dirPin     = 42,
    dirPinInv  = true,
    stepsPerMM = 28.57,
    minSpeed   = 1000,
    maxSpeed   = 60000,
    maxAccel   = 2000,
    maxJerk    = 30000,
    
    endStop = {
              minPin = 51,
              maxPin = 52,
			  homingOnly = false,
              homingDirection = HomingDirectionType.HomingToMin,
              homingSpeed = 1000,
              minHomingPos = 0,
              maxHomingMax = 200, 
              homingRetract = 5,
      }
}

config.axis.z0 = {
	class      = "ZPeterAxis",
    enabled    = true,
    enPin      = 40,
    stepPin    = 13,
    dirPin     = 43,
    dirPinInv  = true,
    stepsPerMM = 24.87638119,
    minSpeed   = 1000,
    maxSpeed   = 15000,
    maxAccel   = 6000,
    maxJerk    = 10000,
    
    endStop = {
              minPin = 53,
              maxPin = 54,
			  homingOnly = true,
              homingDirection = HomingDirectionType.HomingToMin,
              homingSpeed = 1500,
              minHomingPos = 0,
              maxHomingMax = 200, 
              homingRetract = -0.4,
      }
}

config.axis.z1 = {
	enabled = false,
}

config.axis.n0 = {
	enabled = false,
}

config.axis.n1 = {
	enabled = false,
}

config.axis.n2 = {
	enabled = false,
}

config.axis.n3 = {
	enabled = false,
}

-- Actuators
config.actuator.pump = {
	enabled = true,
	class   = "Pump",

	wkPin   = 70,
	stPin   = 69,
	
	vaccSensPin = 35,
	vaccValvePin = 72,
--	
--	commands = {
--		{code = "M814", method = "on"},
--		{code = "M815", method = "off"},
--	},
}

config.actuator.feeder = {
	enabled = true,
	class   = "Feeder",
	commands = {
		{code = "M816", method = "feed"},
		{code = "M817", method = "setId"},
		{code = "M818", method = "setPitch"},
	},
}

-- N0 EXH
config.actuator.n0exh = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 61,
	commands = {
		{code = "M830", method = "on"},
		{code = "M831", method = "off"},
	},
}

-- N0 VACC
config.actuator.n0vacc = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 62,
	commands = {
		{code = "M832", method = "on"},
		{code = "M833", method = "off"},
	},
}

-- N1 EXH
config.actuator.n1exh = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 63,
	commands = {
		{code = "M834", method = "on"},
		{code = "M835", method = "off"},
	},
}

-- N1 VACC
config.actuator.n1vacc = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 64,
	commands = {
		{code = "M836", method = "on"},
		{code = "M837", method = "off"},
	},
}

-- N2 EXH
config.actuator.n2exh = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 65,
	commands = {
		{code = "M838", method = "on"},
		{code = "M839", method = "off"},
	},
}

-- N2 VACC
config.actuator.n2vacc = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 66,
	commands = {
		{code = "M840", method = "on"},
		{code = "M841", method = "off"},
	},
}

-- N3 EXH
config.actuator.n3exh = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 67,
	commands = {
		{code = "M842", method = "on"},
		{code = "M843", method = "off"},
	},
}

-- N3 VACC
config.actuator.n3vacc = {
	enabled = true,
	class   = "Valve",
	ctlPin  = 68,
	commands = {
		{code = "M844", method = "on"},
		{code = "M845", method = "off"},
	},
}

-- Top light
config.actuator.toplight = {
	enabled = true,
	class   = "Light",
	ctlPin  = 9,
	commands = {
		{code = "M850", method = "on"},
		{code = "M851", method = "off"},
	},
}

-- Bottom light
config.actuator.bottomlight = {
	enabled = true,
	class   = "Light",
	ctlPin  = 10,
	commands = {
		{code = "M852", method = "on"},
		{code = "M853", method = "off"},
	},
}