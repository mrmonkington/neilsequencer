enum
{
ENV_ATTACK,
ENV_HOLD,
ENV_DECAY,
ENV_STOP
};

class ahd4
{
public:
	ahd4()
	{
		State = ENV_STOP;
		RtnVal = 0.0f;
		Skip = 0;
		StateCount = 0;
		Increment = 0.0f;
		DecayLen = 0;
		AttackLen = 0;
		HoldLen = 0;
	};
	
	int AttackLen;
	int HoldLen;
	int DecayLen;
	int State;
	int StateCount;
	float Increment;
	float RtnVal;
	int Skip;
	
	inline float ProcEnv()
	{
		RtnVal+=Increment;
		StateCount -= Skip;
		if (StateCount < 0)
		{
			State++;
			switch(State)
			{
				case ENV_HOLD: RtnVal = 1.0f; Increment = 0.0f; StateCount = HoldLen;Skip = 1;break;
				case ENV_DECAY: Increment = -(RtnVal/DecayLen); StateCount = DecayLen; Skip = 1;break;//i dont see how this could work
				case ENV_STOP: Increment = 0.0f; RtnVal = 0.0f; StateCount = 1; Skip = 0;
			}
		}
		return RtnVal;
	}
	
	void SetEnv(int l, float a, float q)
	{
	    //set the envelope lengths
		AttackLen = (int)(l * (a * q));
		DecayLen = (int)(l * (a * (2.0f-q)));
		HoldLen = l - (AttackLen + DecayLen);
		
		//initiate attack
		RtnVal = 0.0f;
		State = ENV_ATTACK;
		Increment = 0.0f;
		if (AttackLen > 0) Increment = 1.0f / (float)AttackLen;
		StateCount = AttackLen;
		Skip = 1;		
	};
};
