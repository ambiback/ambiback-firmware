#ifdef ENABLE_IR
# ifndef irhelper_h
# define irhelper_h

#   include <IRremoteESP8266.h>
#   include <IRrecv.h>

class IRHelper {
public:
    // Constructors
    IRHelper();

    // Declare helper functions
    void checkIRdata(); 
    void enableIrIn(); 
    bool GetIRReceived() { return m_bIRreceived; }
    void SetIRReceived(bool bNew) { m_bIRreceived = bNew; }
    unsigned int GetLastIR() { return m_uilastir; }
    void SetLastIR(unsigned int iNew) { m_uilastir = iNew; }
    unsigned int GetLastReceivedIR() { return m_uilastreceivedir; }
    void SetLastReceivedIR(unsigned int iNew) { m_uilastreceivedir = iNew; }

private:
    // Declare helper variables
    decode_results m_irresults;
    unsigned int m_uilastir;
    unsigned int m_uilastreceivedir;
    bool m_bIRreceived;

    IRrecv* pIRrecv;
};

# endif
#endif 
