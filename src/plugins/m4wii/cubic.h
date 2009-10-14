class Cubic {
public:
  Cubic();
  int Work(int yo, int y0, int y1, int y2, unsigned int res);	
private:
  int RESOLUTION;
  long at[4096];
  long bt[4096];
  long ct[4096];
  long dt[4096];
};
