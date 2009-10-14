#include	"WavetableManager.h"
#include	"Tracker.h"

CWavetableManager::CWavetableManager() {
  m_iNextFreeBuzzSample = 0;
}

CWavetableManager::~CWavetableManager() {

}

IInstrument *CWavetableManager::GetInstrument(int iNum) {
  if (iNum == 0) 
    return 0;
  m_BuzzInstruments[iNum - 1].m_iInsNum = iNum;
  m_BuzzInstruments[iNum - 1].m_pTracker = m_pTracker;
  m_BuzzInstruments[iNum - 1].m_pWaveInfo = m_pTracker->_host->get_wave(iNum);
  if (m_BuzzInstruments[iNum - 1].m_pWaveInfo)
    return &m_BuzzInstruments[iNum - 1];
  else
    return 0;
}

void CWavetableManager::SetTracker(CMatildeTrackerMachine *pTrk) {
  m_pTracker = pTrk;
}

void CWavetableManager::Stop() {
  int i;
  for (i = 0; i < MAX_BUZZSAMPLES; i += 1) {
    m_BuzzSamples[i].Free();
  }
}

CBuzzSample *CWavetableManager::AllocBuzzSample() {
  int i = 0;
  if (m_iNextFreeBuzzSample >= MAX_BUZZSAMPLES)
    m_iNextFreeBuzzSample -= MAX_BUZZSAMPLES;
  while (m_BuzzSamples[m_iNextFreeBuzzSample].m_oUsed) {
    m_iNextFreeBuzzSample += 1;
    if (m_iNextFreeBuzzSample >= MAX_BUZZSAMPLES)
      m_iNextFreeBuzzSample -= MAX_BUZZSAMPLES;
    i += 1;
    if (i >= MAX_BUZZSAMPLES)
      return 0;
  }
  return &m_BuzzSamples[m_iNextFreeBuzzSample++];
}

int CWavetableManager::GetUsedSamples() {
  int i;
  int r = 0;
  for (i = 0; i < MAX_BUZZSAMPLES; i += 1) {
    if (m_BuzzSamples[i].m_oUsed)
      r += 1;
  }
  return r;
}

