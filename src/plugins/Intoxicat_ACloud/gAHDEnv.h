class gAHDEnv;

class gAHDEnv
{
public:
	bool A, H, D;
	float atk, hld, dec, dcs, ddec, datk;
	int atkEnd, decStart, decLen;
	float lastValue;
	//int elen;
	void Init();
	void SetEnvParams(float a, float q);//amoutn, skew value(0-2)
	inline float Envelope(int c, int l, int skip);//counter, envelope length, update freq
	float Envelope2(float c, int skip);//c = EnvCount/EnvLen
	float Envelope3(float c, int skip);
	
};



