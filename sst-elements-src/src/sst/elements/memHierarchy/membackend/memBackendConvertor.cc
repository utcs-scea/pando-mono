// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// Description of modifications made by AMD:
// 1) Memory encryption/decryption latency and access control metadata fetching set in this file
// 2) clock() and doResponse() modified to delay memory requests being sent to memory
// 3) ACM requests modeled by create new memory request and issuing to main memory

// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/memBackendConvertor.h"
#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

MemBackendConvertor::MemBackendConvertor(ComponentId_t id, Params& params, MemBackend* backend, uint32_t request_width) :
    SubComponent(id), m_cycleCount(0), m_reqId(0), m_backend(backend)
{
    m_dbg.init("",
            params.find<uint32_t>("debug_level", 0),
            params.find<uint32_t>("debug_mask", 0),
            (Output::output_location_t)params.find<int>("debug_location", 0 ));

    using std::placeholders::_1;
    m_backend->setGetRequestorHandler( std::bind( &MemBackendConvertor::getRequestor, this, _1 )  );

    m_frontendRequestWidth =  request_width;
    m_backendRequestWidth = static_cast<SimpleMemBackend*>(m_backend)->getRequestWidth();
    if ( m_backendRequestWidth > m_frontendRequestWidth ) {
        m_backendRequestWidth = m_frontendRequestWidth;
    }

    m_clockBackend = m_backend->isClocked();

    stat_GetSReqReceived    = registerStatistic<uint64_t>("requests_received_GetS");
    stat_GetSXReqReceived   = registerStatistic<uint64_t>("requests_received_GetSX");
    stat_GetXReqReceived    = registerStatistic<uint64_t>("requests_received_GetX");
    stat_WriteReqReceived    = registerStatistic<uint64_t>("requests_received_Write");
    stat_PutMReqReceived    = registerStatistic<uint64_t>("requests_received_PutM");
    stat_outstandingReqs    = registerStatistic<uint64_t>("outstanding_requests");
    stat_ReqSent            = registerStatistic<uint64_t>("requests_sent");
    stat_ACMReqSent         = registerStatistic<uint64_t>("acm_requests_sent");
    stat_GetSLatency        = registerStatistic<uint64_t>("latency_GetS");
    stat_GetSXLatency       = registerStatistic<uint64_t>("latency_GetSX");
    stat_GetXLatency        = registerStatistic<uint64_t>("latency_GetX");
    stat_WriteLatency       = registerStatistic<uint64_t>("latency_Write");
    stat_PutMLatency        = registerStatistic<uint64_t>("latency_PutM");

    stat_cyclesWithIssue = registerStatistic<uint64_t>( "cycles_with_issue" );
    stat_cyclesAttemptIssueButRejected = registerStatistic<uint64_t>( "cycles_attempted_issue_but_rejected" );
    stat_totalCycles = registerStatistic<uint64_t>( "total_cycles" );

    stat_GetSReqReceived->setFlagResetCountOnOutput(true);
    stat_GetSXReqReceived->setFlagResetCountOnOutput(true);
    stat_GetXReqReceived->setFlagResetCountOnOutput(true);
    stat_WriteReqReceived->setFlagResetCountOnOutput(true);
    stat_PutMReqReceived->setFlagResetCountOnOutput(true);
    stat_outstandingReqs->setFlagResetCountOnOutput(true);
    stat_ReqSent->setFlagResetCountOnOutput(true);
    stat_ACMReqSent->setFlagResetCountOnOutput(true);
    stat_GetSLatency->setFlagResetCountOnOutput(true);
    stat_GetSXLatency->setFlagResetCountOnOutput(true);
    stat_GetXLatency->setFlagResetCountOnOutput(true);
    stat_WriteLatency->setFlagResetCountOnOutput(true);
    stat_PutMLatency->setFlagResetCountOnOutput(true);
    stat_cyclesWithIssue->setFlagResetCountOnOutput(true);
    stat_cyclesAttemptIssueButRejected->setFlagResetCountOnOutput(true);
    stat_totalCycles->setFlagResetCountOnOutput(true);

    stat_GetSReqReceived->setFlagClearDataOnOutput(true);
    stat_GetSXReqReceived->setFlagClearDataOnOutput(true);
    stat_GetXReqReceived->setFlagClearDataOnOutput(true);
    stat_WriteReqReceived->setFlagClearDataOnOutput(true);
    stat_PutMReqReceived->setFlagClearDataOnOutput(true);
    stat_outstandingReqs->setFlagClearDataOnOutput(true);
    stat_ReqSent->setFlagClearDataOnOutput(true);
    stat_ACMReqSent->setFlagClearDataOnOutput(true);
    stat_GetSLatency->setFlagClearDataOnOutput(true);
    stat_GetSXLatency->setFlagClearDataOnOutput(true);
    stat_GetXLatency->setFlagClearDataOnOutput(true);
    stat_WriteLatency->setFlagClearDataOnOutput(true);
    stat_PutMLatency->setFlagClearDataOnOutput(true);
    stat_cyclesWithIssue->setFlagClearDataOnOutput(true);
    stat_cyclesAttemptIssueButRejected->setFlagClearDataOnOutput(true);
    stat_totalCycles->setFlagClearDataOnOutput(true);

    m_clockOn = true; /* Maybe parent should set this */
    // NOTE: m_secEnabled: 0 - security primitives disabled, 1 - security primitives enabled (needs m_aesEncryptOrDecrypt options to behave correctly)
    m_secEnabled = params.find<uint32_t>("aes_enable_security", 0);
    m_aesDecryptionLat = params.find<uint32_t>("aes_decryption_latency", 5);
    m_acmCheckComputationLat = params.find<uint32_t>("acm_check_latency", 1);
    m_acmRowHit = params.find<uint32_t>("acm_row_hit", 1);
    /* *******
    // NOTE: m_aesEncryptOrDecrypt:
    0 - not defined,
    1 - write encryption latencies only,
    2 - read decryption latencies only,
    3 - encryption+decryption latencies
    ********* */
    m_aesEncryptOrDecrypt = params.find<uint32_t>("aes_encrypt_or_decrypt", 3);
    if (DEBUG_LEVEL > 0) {
        std::cout << "memBackendConverter: AES encryption " << (m_secEnabled ? "ENABLED":"DISABLED") <<
            ", decryption latency: " << m_aesDecryptionLat << " cycles" << "encrypt/decrypt/both: " <<
            m_aesEncryptOrDecrypt << ", ACM check latency: " << m_acmCheckComputationLat <<
            ", ACM row hit: " << m_acmRowHit << std::endl;
    }
    m_acmDependentReqs.clear();
}

void MemBackendConvertor::setCallbackHandlers( std::function<void(Event::id_type,uint32_t)> responseCB, std::function<Cycle_t()> clockenable ) {
    m_notifyResponse = responseCB;
    m_enableClock = clockenable;
}

void MemBackendConvertor::handleMemEvent(  MemEvent* ev ) {

    ev->setDeliveryTime(m_cycleCount);

    doReceiveStat( ev->getCmd() );

    Debug(_L10_,"Creating MemReq. BaseAddr = %" PRIx64 ", Size: %" PRIu32 ", %s\n",
                        ev->getBaseAddr(), ev->getSize(), CommandString[(int)ev->getCmd()]);

    if (!setupMemReq(ev)) {
        ev->acmPassCheck_ = true;
        sendResponse(ev->getID(), ev->getFlags());
    }
}

void MemBackendConvertor::handleCustomEvent( Interfaces::StandardMem::CustomData * info, Event::id_type evId, std::string rqstr) {
    uint32_t id = genReqId();
    CustomReq* req = new CustomReq( info, evId, rqstr, id );
    m_requestQueue.push_back( req );
    m_pendingRequests[id] = req;
}

bool MemBackendConvertor::clock(Cycle_t cycle) {
    m_cycleCount++;

    int reqsThisCycle = 0;
    bool cycleWithIssue = false;

    if (DEBUG_LEVEL > 1) { std::cout << getCurrentSimCycle() << " : m_cycleCount: " << m_cycleCount << std::endl; }

    while (!m_requestQueue.empty()) {

        if ( reqsThisCycle == m_backend->getMaxReqPerCycle() ) {
            break;
        }

        // Note: still performing in-order issue from m_requestQueue, will only pop from m_requestQueue if time has passed
        BaseReq* req = m_requestQueue.front();
        MemEvent* mevent = static_cast<MemReq*>(req)->getMemEvent();

        if (m_secEnabled && (m_aesEncryptOrDecrypt == 1 || m_aesEncryptOrDecrypt == 3)) {
            bool l_needsEncrypt = (static_cast<MemReq*>(req)->isWrite() || (static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushLine) || (static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushLineInv) || (static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushAll) || (static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::PutM));
            if (l_needsEncrypt) {
                // if the aesdonecycle of the write is greater than m_cycleCount (i.e: the encryption time is not done yet)
                if (req->getAesDoneCycle() > m_cycleCount) {
                    // need to add stat for this
                    if (DEBUG_LEVEL > 1) { std::cout << getCurrentSimCycle() << " : req " << req->id() << ", event " << mevent->getID().first << " not complete yet, until cycle " << req->m_aesDoneCycle << std::endl; }
                    cycleWithIssue = false;
                    stat_cyclesAttemptIssueButRejected->addData(1);
                    break;
                }
            }
        }

        if (DEBUG_LEVEL > 0) { std::cout << m_cycleCount << ": req id " << req->id() << ", eventId " << mevent->getID().first << " picked, aesDoneCycle is " << req->getAesDoneCycle() << std::endl; }

        if (DEBUG_LEVEL > 0) { std::cout << getCurrentSimCycle() << ": memBackendConverter clock: event " << mevent->getID().first << " accepted at m_cycleCount " << m_cycleCount << ", aesDoneCycle: " << req->getAesDoneCycle() << std::endl; }
        Debug(_L10_, "Processing request: %s\n", req->getString().c_str());

        // write encryption latency has passed or req is read
        if ( issue( req ) ) {
            cycleWithIssue = true;
        } else {
            cycleWithIssue = false;
            stat_cyclesAttemptIssueButRejected->addData(1);
            break;
        }

        reqsThisCycle++;
        req->increment( m_backendRequestWidth );

        if ( req->issueDone() ) {
            Debug(_L10_, "Completed issue of request\n");
            m_requestQueue.pop_front();
        }
    }

    //if (m_secEnabled && (m_aesEncryptOrDecrypt == 2 || m_aesEncryptOrDecrypt == 3)) {
    if (m_secEnabled) {
        // check if there are read requests ready to be responded to
        while (!m_pendingRequests.empty()) {
            uint32_t id;
            PendingRequests::iterator it;
            for (it = m_pendingRequests.begin(); it != m_pendingRequests.end(); it++) {
                //if (((*it).second->m_aesDoneCycle <= m_cycleCount) && (*it).second->m_fetchedFromMem) {
                if (isDecrypDependencyResolved(static_cast<MemReq*>(it->second)) &&
                    isACMDependencyResolved(static_cast<MemReq*>(it->second))) {
                    // if the decryption time of the read has passed and the request has been fetched from mem, send response
                    id = (*it).second->m_reqId;
                    if (DEBUG_LEVEL > 0) { std::cout << getCurrentSimCycle() << ": clock(): responding in m_cycleCount " << m_cycleCount << " to reqId " << (*it).second->id() << ", baseId: " << (*it).second->getBaseId(it->second->id()) << ", isWrite: " << (static_cast<MemReq*>(m_pendingRequests[id])->isWrite() ? "TRUE":"FALSE") << ", m_aesDoneCycle: " << (*it).second->m_aesDoneCycle << std::endl; }
                    //fprintf(stderr, "responding to Req: %p id: %u @ %lu\n", it->second, it->first, m_cycleCount);
                    break;
                }
            }

            if (it != m_pendingRequests.end()) {
                BaseReq* req = m_pendingRequests[id];
                MemEvent* event = static_cast<MemReq*>(req)->getMemEvent();
                m_pendingRequests.erase(id);
                uint32_t flags = event->getFlags();
                if (DEBUG_LEVEL > 0 ) {
                    std::cout << "clock/sendResponse: GetS: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetS) ? "TRUE,":"FALSE,") \
                    << "\t GetX: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetX) ? "TRUE,":"FALSE,") \
                    << "\t GetSX: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetSX) ? "TRUE,":"FALSE,") \
                    << "\t Write: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::Write) ? "TRUE,":"FALSE,") \
                    << "\t FlushLine: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushLine) ? "TRUE,":"FALSE,") \
                    << "\t FlushLineInv: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushLineInv) ? "TRUE,":"FALSE,") \
                    << "\t FlushAll: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushAll) ? "TRUE,":"FALSE,") \
                    << "\t GetSResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetSResp) ? "TRUE,":"FALSE,") \
                    << "\t GetXResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetXResp) ? "TRUE,":"FALSE,") \
                    << "\t WriteResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::WriteResp) ? "TRUE,":"FALSE,") \
                    << "\t FlushLineResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushLineResp) ? "TRUE,":"FALSE,") \
                    << "\t FlushAllResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FlushAllResp) ? "TRUE,":"FALSE,") \
                    << "\t FetchResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FetchResp) ? "TRUE,":"FALSE,") \
                    << "\t FetchXResp: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::FetchXResp) ? "TRUE,":"FALSE,") \
                    << "\t NACK: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::NACK) ? "TRUE,":"FALSE,") \
                    << "\t AckInv: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::AckInv) ? "TRUE,":"FALSE,") \
                    << "\t AckPut: " << ((static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::AckPut) ? "TRUE,":"FALSE,") << std::endl;;
                }

                if (!req->isMemEv()) {
                    CustomReq* creq = static_cast<CustomReq*>(req);
                    sendResponse(creq->getEvId(), req->m_flags);
                } else {

                    Debug(_L10_,"doResponse req is done. %s\n", event->getBriefString().c_str());

                    Cycle_t latency = m_cycleCount - event->getDeliveryTime();

                    Command cmd = event->getCmd();

                    doResponseStat( event->getCmd(), latency );

                    // TODO: check on this, flags is passed as a parameter to doResponse, may need additional member for these flags
                    uint32_t flags = req->m_flags;

                    SST::Event::id_type evID = event->getID();
                    if (DEBUG_LEVEL > 0) { std::cout << getCurrentSimCycle() << ": clock(): req->isDone() is true, calling sendResponse in m_cycleCount " << m_cycleCount << " for eventId " << event->getID().first << ", reqId: " << req->id() << ", baseId: " << id << " ,isWrite: " << (static_cast<MemReq*>(req)->isWrite() ? "TRUE":"FALSE") << std::endl; }

                    event->setAcmPassCheck(true);
                    sendResponse(event->getID(), flags); // Needs to occur before a flush is completed since flush is dependent

                    if (m_dependentRequests.find(evID) != m_dependentRequests.end()) {
                        std::set<MemEvent*, memEventCmp> flushes = m_dependentRequests.find(evID)->second;

                        for (std::set<MemEvent*, memEventCmp>::iterator it = flushes.begin(); it != flushes.end(); it++) {
                            (m_waitingFlushes.find(*it)->second).erase(evID);
                            if ((m_waitingFlushes.find(*it)->second).empty()) {
                                MemEvent * flush = *it;
                                sendResponse(flush->getID(), flush->getFlags());
                                m_waitingFlushes.erase(flush);
                            }
                        }
                        m_dependentRequests.erase(evID);
                    }
                }
                delete req;
            } else {
                // an eligible request was not found, continue clock()
                break;
            } // end if (it != m_pendingRequests.end())
        } // end while (!m_pendingRequests.empty())
    } // end if (m_secEnabled)

    if (cycleWithIssue)
        stat_cyclesWithIssue->addData(1);

    stat_outstandingReqs->addData( m_pendingRequests.size() );

    bool unclock = !m_clockBackend;
    if (m_clockBackend)
        unclock = m_backend->clock(cycle);

    // Can turn off the clock if:
    // 1) backend says it's ok
    // 2) requestQueue is empty
    if (unclock && m_requestQueue.empty())
        return true;

    return false;
}

/*
 * Called by MemController to turn the clock back on
 * cycle = current cycle
 */
void MemBackendConvertor::turnClockOn(Cycle_t cycle) {
    Cycle_t cyclesOff = cycle - m_cycleCount;
    stat_outstandingReqs->addDataNTimes( cyclesOff, m_pendingRequests.size() );
    m_cycleCount = cycle;
    m_clockOn = true;
}

/*
 * Called by MemController to turn the clock off
 */
void MemBackendConvertor::turnClockOff() {
    m_clockOn = false;
}

void MemBackendConvertor::doResponse( ReqId reqId, uint32_t flags ) {

    /* If clock is not on, turn it back on */
    if (!m_clockOn) {
        Cycle_t cycle = m_enableClock();
        turnClockOn(cycle);
    }

    // if isWrite, then perform the below actions immediately, else, set time of arrival (i.e: first time doResponse was called) then return
    // when doResponse called in clock(), assert that m_cycleCount >= timeOfArrival (m_cycleCount += m_aes_DecryptionLat)

    if (DEBUG_LEVEL > 0) { std::cout << getCurrentSimCycle() << ": doResponse for reqId " << reqId <<", m_cycleCount: " << m_cycleCount << std::endl; }
    uint32_t id = BaseReq::getBaseId(reqId);
    MemEvent* resp = NULL;

    if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
        m_dbg.fatal(CALL_INFO, -1, "memory request not found; id=%" PRId32 "\n", id);
    }

    BaseReq* req = m_pendingRequests[id];
    req->m_fetchedFromMem = true;

    if (m_secEnabled && (req->isACMReq)) {
        // TODO: how will the acmPassCheck be set? need to check the value of the data itself
        //process and delete the ACM req
        if (DEBUG_LEVEL > 0) {
            fprintf(stderr, "got ACM Response for: %p id: %u for actual req: %p @ %lu\n",req, id, m_acmDependentReqs[req], m_cycleCount);
        }
        // erase pending acm req
        m_pendingRequests.erase(id);
        m_acmDependentReqs[req]->m_waitOnACM--;

        if (!m_acmDependentReqs[req]->m_waitOnACM) {
            //TODO parameterize delay for checking ACM
            m_acmDependentReqs[req]->m_acmDoneCycle = m_cycleCount + m_acmCheckComputationLat;

            // m_acmDependentReqs[req]->setAcmPassCheck(true);
            m_acmDependentReqs[req]->setAcmPassCheck(true);

            // Add decryption latency to ACM request
            // TODO check aesDecry option
            if (m_aesEncryptOrDecrypt == 2 || m_aesEncryptOrDecrypt == 3) {
                m_acmDependentReqs[req]->m_acmDoneCycle += m_aesDecryptionLat;
            }
        } else {
            // for some transactions, like cacheline flush, ACM is read and stored back after
            // modification, so issue another ACM req
            int32_t acmReq_id, numACMReqs;
            MemReq* acmReq = nullptr;
            //TODO only generate acmReq if main req has userID
            acmReq_id = genReqId();
            // TODO modify the address ?
            MemEvent *acmMemEvnt = new MemEvent(*(static_cast<MemReq*>(m_acmDependentReqs[req])->getMemEvent()));
            acmMemEvnt->setCmd(Command::Write);

            //acm results in row miss
            if (!m_acmRowHit) {
                Addr origAddr = acmMemEvnt->getBaseAddr();
                if (DEBUG_LEVEL > 0) {
                    fprintf(stderr, "write Address changed from %lx to %lx\n", origAddr,(origAddr ^ 0xFFFFFFFF));
                }
                //flipping the address bits
                acmMemEvnt->setBaseAddr(origAddr ^ 0xFFFFFFFF);
            }

            //TODO change address, based on mapper
            acmReq = new MemReq( acmMemEvnt, acmReq_id );
            acmReq->isACMReq = true;
            // delay added for encryption
            if (m_aesEncryptOrDecrypt == 1 || m_aesEncryptOrDecrypt == 3) {
                req->m_aesDoneCycle = m_cycleCount + m_aesDecryptionLat;
            }
            m_requestQueue.push_back(acmReq);
            m_pendingRequests[acmReq_id] = acmReq;
            m_acmDependentReqs[acmReq] = m_acmDependentReqs[req];

            //Stat
            stat_ReqSent->addData(1);
            stat_ACMReqSent->addData(1);

            assert(m_acmDependentReqs[req]->m_waitOnACM == 1);
            if (DEBUG_LEVEL > 0) {
                fprintf(stderr, "pushing ACM ST Req: %p id: %u first ACM req: %p for actual req: %p @ %lu\n", acmReq, acmReq_id, req, m_acmDependentReqs[req], m_cycleCount);
            }
        }

        //erase first ACM req dependency
        m_acmDependentReqs.erase(req);
        delete req;

        return;
    }

    // for reads: set the fetchedFromMem flag for both read/writes but add latency to reads, response is sent in clock() function
    if (m_secEnabled && ((m_aesEncryptOrDecrypt == 2) ||
                         (m_aesEncryptOrDecrypt == 3) ||
                         (!isACMDependencyResolved(req)))) {

        if (DEBUG_LEVEL > 0) {
            fprintf(stderr, "got Response (not sending resp up) for: %p id: %u @ %lu\n", req, id, m_cycleCount);
        }

        // actual response done as part of clock() function
        // set flags
        req->m_flags = flags;
        if (!req->isMemEv()) {
            if (!flags) {
                req->m_flags = static_cast<MemReq*>(req)->getMemEvent()->getFlags();
            }
        }
        // to indicate that the request in the queue has been serviced and response can be issued in clock
        req->m_fetchedFromMem = true;
        bool l_isRead = (static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetS || static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetSX || static_cast<MemReq*>(req)->getMemEvent()->getCmd() == Command::GetX);
        // set ready time
        if (l_isRead) {
            req->m_aesDoneCycle = m_cycleCount + m_aesDecryptionLat;
            if (DEBUG_LEVEL > 0) { std::cout << getCurrentSimCycle() << ": fetched read req " << reqId << ", baseId: " << id << " in cycle " << m_cycleCount << ", setting aesDoneCycle to " << req->m_aesDoneCycle << std::endl;}
        }
    } else {
        if (DEBUG_LEVEL > 0) {
            fprintf(stderr, "got Response (sending resp up) for: %p id: %u @ %lu\n", req, id, m_cycleCount);
        }
        // default behavior w/o encryption enabled
        req->decrement( );

        if ( req->isDone() ) {
            m_pendingRequests.erase(id);

            if (!req->isMemEv()) {
                CustomReq* creq = static_cast<CustomReq*>(req);
                sendResponse(creq->getEvId(), flags);
            } else {

                MemEvent* event = static_cast<MemReq*>(req)->getMemEvent();

                Debug(_L10_,"doResponse req is done. %s\n", event->getBriefString().c_str());

                Cycle_t latency = m_cycleCount - event->getDeliveryTime();

                doResponseStat( event->getCmd(), latency );

                if (!flags) flags = event->getFlags();
                SST::Event::id_type evID = event->getID();

                sendResponse(event->getID(), flags); // Needs to occur before a flush is completed since flush is dependent

                if (m_dependentRequests.find(evID) != m_dependentRequests.end()) {
                    std::set<MemEvent*, memEventCmp> flushes = m_dependentRequests.find(evID)->second;

                    for (std::set<MemEvent*, memEventCmp>::iterator it = flushes.begin(); it != flushes.end(); it++) {
                        (m_waitingFlushes.find(*it)->second).erase(evID);
                        if ((m_waitingFlushes.find(*it)->second).empty()) {
                            MemEvent * flush = *it;
                            sendResponse(flush->getID(), flush->getFlags());
                            m_waitingFlushes.erase(flush);
                        }
                    }
                    m_dependentRequests.erase(evID);
                }
            }
            delete req;
        }
    }
}

void MemBackendConvertor::sendResponse( SST::Event::id_type id, uint32_t flags ) {

    m_notifyResponse( id, flags );

}

void MemBackendConvertor::finish(Cycle_t endCycle) {
    // endCycle can be less than m_cycleCount in parallel simulations
    // because the simulation end isn't detected until a sync interval boundary
    // and endCycle is adjusted to the actual (not detected) end time
    // stat_outstandingReqs may vary slightly in parallel & serial
    if (endCycle > m_cycleCount) {
        Cycle_t cyclesOff = endCycle - m_cycleCount;
	stat_outstandingReqs->addDataNTimes( cyclesOff, m_pendingRequests.size());
        m_cycleCount = endCycle;
    }
    stat_totalCycles->addData(m_cycleCount);
    m_backend->finish();
}

size_t MemBackendConvertor::getMemSize() {
    return m_backend->getMemSize();
}

uint32_t MemBackendConvertor::getRequestWidth() {
    return m_backend->getRequestWidth();
}
