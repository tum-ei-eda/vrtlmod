/*
 * Copyright 2021 Chair of EDA, Technical University of Munich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file mif2tlm_bridge.h
/// @date 2022-08-24
/// @brief memory interface to tlm (mif2tlm) bridge inspired by axi2tlm bridges
/// (at: https://github.com/Xilinx/libsystemctlm-soc.git)
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CV32E40S_VP_MIF2TLM_BRIDGE_H__
#define __CV32E40S_VP_MIF2TLM_BRIDGE_H__

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc>
#include "tlm_utils/simple_initiator_socket.h"
#include <list>
#include "transaction_commons.h"

template <int ADDR_WIDTH, int DATA_WIDTH, int BYTE_EN_WIDTH, int ACHK_WIDTH = 12, int MEMTYPE_WIDTH = 2,
          int PROT_WIDTH = 3, int RCHK_WIDTH = 5>
class mif_signals : public sc_core::sc_module
{
  public:
    // addr, request, grant = commons
    sc_signal<bool> req{ "req" };
    sc_signal<bool> reqpar{ "reqpar" }; // odd parity req

    sc_signal<sc_bv<ADDR_WIDTH>> addr{ "addr" };
    sc_signal<sc_bv<ACHK_WIDTH>> achk{ "achk" }; // checksum address phase signal

    sc_signal<bool> gnt{ "gnt" };
    sc_signal<bool> gntpar{ "gntpar" }; // odd parity gnt

    sc_signal<sc_bv<MEMTYPE_WIDTH>> memtype{ "memtype" };
    sc_signal<sc_bv<PROT_WIDTH>> prot{ "prot" };
    sc_signal<bool> dbg{ "dbg" };
    sc_signal<bool> err{ "err" };

    // read channel
    sc_signal<bool> rvalid{ "rvalid" };
    sc_signal<bool> rvalidpar{ "rvalidpar" }; // odd parity valid

    sc_signal<sc_bv<DATA_WIDTH>> rdata{ "rdata" };
    sc_signal<sc_bv<RCHK_WIDTH>> rchk{ "rchk" };

    // write channel
    sc_signal<bool> we{ "we" };
    sc_signal<sc_bv<BYTE_EN_WIDTH>> be{ "be" };
    sc_signal<sc_bv<DATA_WIDTH>> wdata{ "wdata" };

    template <typename T>
    void connect(T &dev, const char *prefix)
    {
        signal_connect(&dev, prefix, req);
        signal_connect(&dev, prefix, reqpar);

        signal_connect(&dev, prefix, addr);
        signal_connect(&dev, prefix, achk);

        signal_connect(&dev, prefix, gnt);
        signal_connect(&dev, prefix, gntpar);

        signal_connect(&dev, prefix, memtype);
        signal_connect(&dev, prefix, prot);
        signal_connect(&dev, prefix, dbg);
        signal_connect(&dev, prefix, err);

        signal_connect(&dev, prefix, rvalid);
        signal_connect(&dev, prefix, rvalidpar);

        signal_connect(&dev, prefix, rdata);
        signal_connect(&dev, prefix, rchk);

        signal_connect_optional(&dev, prefix, we);
        signal_connect_optional(&dev, prefix, be);
        signal_connect_optional(&dev, prefix, wdata);
    }

    template <typename T>
    void connect(T *dev)
    {
        dev->req(req);
        dev->reqpar(reqpar);

        dev->addr(addr);
        dev->achk(achk);

        dev->gnt(gnt);
        dev->gntpar(gntpar);

        dev->memtype(memtype);
        dev->prot(prot);
        dev->dbg(dbg);
        dev->err(err);

        dev->rvalid(rvalid);
        dev->rvalidpar(rvalidpar);

        dev->rdata(rdata);
        dev->rchk(rchk);

        dev->we(we);
        dev->be(be);
        dev->wdata(wdata);
    }

    template <typename T>
    void connect(T &dev)
    {
        connect(&dev);
    }

    mif_signals(sc_core::sc_module_name name) {}
};

class mif_common
{
  public:
    template <typename T>
    mif_common(T *mod) : clk(mod->clk), resetn(mod->resetn)
    {
    }

    mif_common(sc_in<bool> &_clk, sc_in<bool> &_resetn) : clk(_clk), resetn(_resetn) {}

    void wait_for_reset_release()
    {
        do
        {
            sc_core::wait(clk.posedge_event());
        } while (resetn.read() == false);
    }

    bool reset_asserted() { return resetn.read() == false; }

    void wait_abort_on_reset(sc_in<bool> &sig)
    {
        do
        {
            sc_core::wait(clk.posedge_event() | resetn.negedge_event());
        } while (sig.read() == false && resetn.read() == true);
    }

    template <typename T>
    void tlm2mif_clear_fifo(sc_fifo<T *> &fifo)
    {
        while (fifo.num_available() > 0)
        {
            T *tr = fifo.read();

            abort(tr);
        }
    }

    template <typename T>
    void mif2tlm_clear_fifo(sc_fifo<T *> &fifo)
    {
        while (fifo.num_available() > 0)
        {
            T *tr = fifo.read();
            delete tr;
        }
    }

    template <typename T>
    void abort(T *tr)
    {
        tlm::tlm_generic_payload &trans = tr->GetGP();

        trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        tr->DoneEvent().notify();
    }

  private:
    sc_in<bool> &clk;
    sc_in<bool> &resetn;
};

template <int ADDR_WIDTH, int DATA_WIDTH, int BYTE_EN_WIDTH, int ACHK_WIDTH = 12, int MEMTYPE_WIDTH = 2,
          int PROT_WIDTH = 3, int RCHK_WIDTH = 5, bool BYTE_EN = false, bool READ_ONLY = false>
class mif2tlm_bridge : public sc_core::sc_module, public mif_common
{
  public:
    SC_HAS_PROCESS(mif2tlm_bridge);

    sc_in<bool> clk{ "clk" };
    sc_in<bool> resetn{ "resetn" };
    tlm_utils::simple_initiator_socket<mif2tlm_bridge> socket_;

    sc_in<bool> req{ "req" };
    sc_in<bool> reqpar{ "reqpar" };

    sc_out<bool> gnt{ "gnt" };
    sc_out<bool> gntpar{ "gntpar" };

    sc_out<bool> rvalid{ "rvalid" };
    sc_out<bool> rvalidpar{ "rvalidpar" };

    sc_in<bool> we{ "we" };
    sc_in<sc_bv<BYTE_EN_WIDTH>> be{ "be" };

    sc_in<sc_bv<ADDR_WIDTH>> addr{ "addr" };
    sc_in<sc_bv<ACHK_WIDTH>> achk{ "achk" };

    sc_in<sc_bv<MEMTYPE_WIDTH>> memtype{ "memtype" };
    sc_in<sc_bv<PROT_WIDTH>> prot{ "prot" };
    sc_in<bool> dbg{ "dbg" };

    sc_in<sc_bv<DATA_WIDTH>> wdata{ "wdata" };

    sc_out<sc_bv<DATA_WIDTH>> rdata{ "rdata" };
    sc_out<sc_bv<RCHK_WIDTH>> rchk{ "rchk" };

    sc_out<bool> err{ "err" };

    static const uint32_t DATA_BUS_BYTES = DATA_WIDTH / 8;
    uint64_t num_transactions_;

    mif2tlm_bridge(sc_core::sc_module_name name) : sc_module(name), mif_common(this), num_transactions_(0)
    {
        SC_THREAD(update_process);

        SC_THREAD(reset);

        SC_METHOD(scan_req_lines);
        sensitive << req;
        SC_METHOD(err_det);
        sensitive << err;
    }

    void scan_req_lines(void)
    {
        gnt.write(req.read());
        gntpar.write(~req.read());
    }

    void err_det(void) { std::cout << "obi err: " << err << " achk:" << achk << " addr:" << addr << std::endl; }
    ~mif2tlm_bridge(void){};

  private:
    class Transaction
    {
        tlm::tlm_generic_payload *gp_;
        uint64_t alignedAddress_;
        uint32_t dataIdx_;
        uint32_t transaction_id_;
        sc_time delay_;
        bool tlm_ongoing_;

      public:
        uint64_t align(uint64_t addr, uint64_t alignTo) { return ((addr / alignTo) * alignTo); }

        Transaction(tlm::tlm_command cmd, uint64_t address, uint8_t numberBytes, uint32_t transaction_id, int be_flags)
            : gp_(prepare_trans(numberBytes, !BYTE_EN ? -1 : be_flags))
            , alignedAddress_(address)
            , dataIdx_(0)
            , transaction_id_(transaction_id)
            , tlm_ongoing_(false)
            , delay_(SC_ZERO_TIME)
        {
            auto len = get_data_len(address, alignedAddress_, numberBytes);
            assert(numberBytes > 0);
            assert(len > 0);

            gp_->set_command(cmd);
            // align the address to 4-byte word - offsets are resolved through byte enable in ri5cy lsu
            address &= ~((uint64_t)(0x3));
            gp_->set_address(address);
            gp_->set_dmi_allowed(false);
            gp_->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        }

        uint32_t get_data_len(uint64_t address, uint64_t alignedAddress, uint8_t numberBytes)
        {
            return (numberBytes - (address - alignedAddress));
        }
        unsigned int GetDataLen(void) { return (gp_->get_data_length()); }
        tlm::tlm_command GetCommand(void) { return gp_->get_command(); }
        uint64_t GetAddress(void) { return gp_->get_address(); }
        uint32_t GetTransactionID(void) { return (transaction_id_); }
        tlm::tlm_response_status GetTLMResponse(void) { return (gp_->get_response_status()); }
        void SetTLMOngoing(bool tlm_ongoing = true) { tlm_ongoing_ = tlm_ongoing; }
        bool TLMOngoing(void) { return (tlm_ongoing_); }

        template <typename T1>
        void FillData(T1 &wdata)
        {
            unsigned char *gp_data = gp_->get_data_ptr();

            for (int byte_i = 0; byte_i < gp_->get_data_length(); ++byte_i)
            {
                int firstbit = byte_i * 8;
                int lastbit = firstbit + 8 - 1;
                gp_data[byte_i] = wdata.read().range(lastbit, firstbit).to_uint();
            }
        }

        template <typename T>
        void GetData(T &data)
        {
            unsigned char *gp_data = gp_->get_data_ptr();
            // Set data
            for (int byte_i = 0; byte_i < gp_->get_data_length(); ++byte_i)
            {
                int firstbit = byte_i * 8;
                int lastbit = firstbit + 8 - 1;
                data.range(lastbit, firstbit) = gp_data[byte_i];
            }
        }

        tlm::tlm_generic_payload *GetTLMGenericPayload(void) { return (gp_); }

    }; /* class Transaction */

    sc_fifo<Transaction *> fifo_;

    std::list<Transaction *> req_list_;

    Transaction *GetFirstWithID(std::list<Transaction *> *list, uint32_t transaction_id)
    {
        for (typename std::list<Transaction *>::iterator it = list->begin(); it != list->end(); it++)
        {
            Transaction *t = (*it);

            if (t && t->GetTransactionID() == transaction_id)
            {
                return t;
            }
        }
        return NULL;
    }

    bool InList(std::list<Transaction *> &l, uint32_t transaction_id)
    {
        for (typename std::list<Transaction *>::iterator it = l.begin(); it != l.end(); it++)
        {
            if ((*it)->GetTransactionID() == transaction_id)
            {
                return true;
            }
        }
        return false;
    }
    bool PreviousHaveData(std::list<Transaction *> &l, uint32_t id) { return (true); }
    bool OverlappingAddress(std::list<Transaction *> *list, Transaction *tr)
    {
        for (typename std::list<Transaction *>::iterator it = list->begin(); *it != tr; it++)
        {
            uint64_t address1 = (*it)->GetTLMGenericPayload()->get_address();
            uint64_t address2 = tr->GetTLMGenericPayload()->get_address();

            if ((address1 >> 12) == (address2 >> 12))
            {
                return true;
            }
        }
        return false;
    }
    bool WaitForTransactions(std::list<Transaction *> *list, Transaction *tr) { return OverlappingAddress(list, tr); }

  private:
    /* Threads */

    template <typename int_t>
    bool get_parity(int_t n)
    {
        bool parity = 0;
        while (n)
        {
            parity = !parity;
            n = n & (n - 1);
        }
        return parity;
    }

    void update_process(void)
    {
        while (true)
        {
            bool error_flag = false;
            uint8_t _rchk = 0;
            wait(clk.posedge_event());

            auto _req = req.read();
            auto _reqpar = reqpar.read();

            if (_req ^ _reqpar != 1)
            {
                error_flag = true;
            }

            rvalid.write(req);
            rvalidpar.write(~req);

            if (_req)
            {

                uint32_t _addr = addr.read().to_uint();
                uint16_t _achk = achk.read().to_uint();

                bool achk_ok = true;

                achk_ok &= (get_parity(_addr & 0xFF) ^ (_achk & 0x1));
                achk_ok &= (get_parity(_addr & 0xFF00) ^ ((_achk >> 1) & 0x1));
                achk_ok &= (get_parity(_addr & 0xFF0000) ^ ((_achk >> 2) & 0x1));
                achk_ok &= (get_parity(_addr & 0xFF000000) ^ ((_achk >> 3) & 0x1));

                uint8_t _memtype = memtype.read().to_uint();
                uint8_t _prot = prot.read().to_uint();
                achk_ok &= (((get_parity(_memtype << PROT_WIDTH | _prot) ^ ((_achk >> 4) & 0x1))) == 1);

                uint8_t _be = BYTE_EN ? be.read().to_uint() : 0xf;
                uint8_t _we = we.read();
                achk_ok &= (((get_parity(_be << 1 | _we) ^ ((_achk >> 5) & 0x1))) == 1);

                achk_ok &= ((dbg.read() ^ ((_achk >> 6) & 0x1)) == 1);

                achk_ok &= (((_achk >> 7) & 0x1));

                uint32_t _wdata = READ_ONLY ? 0 : wdata.read().to_uint();
                achk_ok &= (get_parity(_wdata & 0xFF) ^ ((_achk >> 8) & 0x1));
                achk_ok &= (get_parity(_wdata & 0xFF00) ^ ((_achk >> 9) & 0x1));
                achk_ok &= (get_parity(_wdata & 0xFF0000) ^ ((_achk >> 10) & 0x1));
                achk_ok &= (get_parity(_wdata & 0xFF000000) ^ ((_achk >> 11) & 0x1));

                if (!achk_ok)
                {
                    error_flag = true;
                }

                Transaction t(_we ? tlm::TLM_WRITE_COMMAND : tlm::TLM_READ_COMMAND, addr.read().to_uint64(),
                              DATA_BUS_BYTES, num_transactions_++, be.read().to_uint());

                if (_we)
                {
                    t.FillData(wdata);
                }

                sc_time delay(SC_ZERO_TIME);
                tlm::tlm_generic_payload *gp = t.GetTLMGenericPayload();
                t.SetTLMOngoing();
                socket_->b_transport(*gp, delay);
                wait(delay, resetn.negedge_event());
                t.SetTLMOngoing(false);

                if (!_we)
                {
                    sc_bv<DATA_WIDTH> tmp = rdata;
                    t.GetData(tmp);
                    rdata.write(tmp);

                    uint32_t _rdata = rdata.read().to_uint();

                    _rchk = (error_flag << 4 | (get_parity(_rdata & 0xFF000000) << 3) |
                             (get_parity(_rdata & 0xFF0000) << 2) | (get_parity(_rdata & 0xFF00) << 1) |
                             get_parity(_rdata & 0xFF));
                }
            }
            rchk.write(_rchk);
            err.write(error_flag);
        }
    }

    void TLMListClear(std::list<Transaction *> &l)
    {
        // Schedule abort on transactions that are ongoing and abort
        // all others
        for (typename std::list<Transaction *>::iterator it = l.begin(); it != l.end();)
        {
            Transaction *t = (*it);
            it = l.erase(it);
            delete t;
        }
    }

    void reset(void)
    {
        while (true)
        {
            wait(resetn.negedge_event());

            mif2tlm_clear_fifo(fifo_);

            TLMListClear(req_list_);

            num_transactions_ = 0;
        }
    }
};

#endif /* __CV32E40S_VP_MIF2TLM_BRIDGE_H__ */
