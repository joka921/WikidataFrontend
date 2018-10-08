#ifndef _QLEVER_RESULT_H
#define _QLEVER_RESULT_H

#include <vector>
#include <string>

// ________________________________________________________________________________
class QLeverResult {
 public:
  std::string status;
  std::vector<std::vector<std::string>> res;
};



#endif  // _QLEVER_RESULT_H
