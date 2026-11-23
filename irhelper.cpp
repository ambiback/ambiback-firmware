// Include all needed files
#include "globalincludes.h"
#ifdef ENABLE_IR
#include "irhelper.h"

IRHelper::IRHelper(){
  m_uilastir = 0;
  m_uilastreceivedir = 0;
  m_bIRreceived = false; 
  
  pIRrecv = new IRrecv(IRRECV_PIN);
}

void IRHelper::enableIrIn() {
  // Start IR Receiver First!
  pIRrecv->enableIRIn();
}

void IRHelper::checkIRdata() {
  /* Check for IR data */
  if (!m_bIRreceived && pIRrecv->decode(&m_irresults)) {
    
    _DBGPRINTF(PSTR("Received IR Type: 0x%08x, Value: 0x%08x, Address: 0x%08x, Command: 0x%08x\n"), 
      m_irresults.decode_type,
      m_irresults.value,
      m_irresults.address,
      m_irresults.command);

    // Get Last Value
    m_uilastreceivedir = m_irresults.value; 
    
    pIRrecv->resume(); // Receive the next value
    m_bIRreceived = true;
  }
}
#endif
