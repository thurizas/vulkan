#ifndef _utilities_h_
#define _utilities_h_

struct queueFamilyIndices {
  int graphicsFamily = -1;
  int presentationFamily = -1;

  bool isValid()
  {
    return( (graphicsFamily >= 0) && (presentationFamily >= 0));
  }
};

#endif