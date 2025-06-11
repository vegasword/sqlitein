PerfCounter InitPerfCounter(void)
{
  PerfCounter pc = (PerfCounter){0};
  QueryPerformanceFrequency(&pc.frequency);
  return pc;
}

void StartPerfCounter(PerfCounter *pc)
{
  QueryPerformanceCounter(&pc->t1);
}

void EndPerfCounter(PerfCounter *pc)
{
  QueryPerformanceCounter(&pc->t2);  
  pc->elapsedMilliseconds = ((pc->t2.QuadPart - pc->t1.QuadPart) * 1000) / pc->frequency.QuadPart;
}

f32 GetDeltaTime(PerfCounter *pc)
{
  EndPerfCounter(pc);
  pc->t1.QuadPart = pc->t2.QuadPart;
  return (f32)pc->elapsedMilliseconds;
}
