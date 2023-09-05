#ifndef _vkException_h_
#define _vkException_h_

#include <exception>

struct vkException : public std::exception
{
 /************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
  vkException(VkResult val, const char* f, const char* e) : m_res(val), m_fnct(f), m_err(e)
  {

  }



/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   : Aug 2023 (GKHuber)
************************************************************************************************************************/
  const char* what() const noexcept override
  {
    std::string msg = "in function: " + std::string(m_fnct) + " error code: " + std::to_string(m_res) + " desc: " + std::string(m_err);
    return msg.c_str();
  }

private:
  VkResult    m_res;
  const char* m_fnct;
  const char* m_err;

};



#endif