// Modified version of fast-strcmp by Markus Grönholm, 2014
// @ https://mgronhol.github.io/fast-strcmp/
// @ https://gist.github.com/mgronhol/018e17c118ccf2144744#file-fast-str-c
i32 FastStrCmp(const char *ptr0, const char *ptr1, size_t length)
{
  i32 fast = (i32)length / sizeof(size_t) + 1;
  i32 offset = (fast - 1) * sizeof(size_t);
  i32 current_block = 0;

  size_t *lptr0 = (size_t *)ptr0;
  size_t *lptr1 = (size_t *)ptr1;

  while (current_block < fast)
  {
    if (lptr0[current_block] ^ lptr1[current_block])
    {
      i32 pos;
      for (pos = current_block * sizeof(size_t); pos < length; ++pos)
      {
        if ((ptr0[pos] ^ ptr1[pos]) || (ptr0[pos] == 0) || (ptr1[pos] == 0))
        {
          return (i32)((uc)ptr0[pos] - (uc)ptr1[pos]);
        }
      }
    }

    ++current_block;
  }

  while (length > offset)
  {
    if (ptr0[offset] ^ ptr1[offset])
    {
      return (i32)((uc)ptr0[offset] - (uc)ptr1[offset]); 
    }
    ++offset;
  }
	
   return 0;
}
