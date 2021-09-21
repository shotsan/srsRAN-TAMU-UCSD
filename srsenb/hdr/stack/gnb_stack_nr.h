/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/******************************************************************************
 * File:        gnb_stack_nr.h
 * Description: L2/L3 gNB stack class.
 *****************************************************************************/

#ifndef SRSRAN_GNB_STACK_NR_H
#define SRSRAN_GNB_STACK_NR_H

#include "srsenb/hdr/stack/mac/nr/mac_nr.h"
#include "srsenb/hdr/stack/rrc/rrc_nr.h"
#include "upper/pdcp.h"
#include "upper/rlc.h"
#include "upper/sdap.h"

#include "enb_stack_base.h"
#include "srsran/interfaces/gnb_interfaces.h"

namespace srsenb {

class gnb_stack_nr final : public srsenb::enb_stack_base,
                           public stack_interface_phy_nr,
                           public stack_interface_mac,
                           public srsue::stack_interface_gw,
                           public rrc_nr_interface_rrc,
                           public pdcp_interface_gtpu, // for user-plane over X2
                           public srsran::thread
{
public:
  explicit gnb_stack_nr(srslog::sink& log_sink);
  ~gnb_stack_nr() final;

  int init(const srsenb::stack_args_t& args_,
           const rrc_nr_cfg_t&         rrc_cfg_,
           phy_interface_stack_nr*     phy_,
           x2_interface*               x2_);

  // eNB stack base interface
  void        stop() final;
  std::string get_type() final;
  bool        get_metrics(srsenb::stack_metrics_t* metrics) final;

  // GW srsue stack_interface_gw dummy interface
  bool is_registered() override { return true; };
  bool start_service_request() override { return true; };

  // Temporary GW interface
  void write_sdu(uint32_t lcid, srsran::unique_byte_buffer_t sdu) override;
  bool has_active_radio_bearer(uint32_t eps_bearer_id) override;
  bool switch_on();
  void tti_clock() override;

  // MAC interface to trigger processing of received PDUs
  void process_pdus() final;

  void toggle_padding() override {}

  int  slot_indication(const srsran_slot_cfg_t& slot_cfg) override;
  int  get_dl_sched(const srsran_slot_cfg_t& slot_cfg, dl_sched_t& dl_sched) override;
  int  get_ul_sched(const srsran_slot_cfg_t& slot_cfg, ul_sched_t& ul_sched) override;
  int  pucch_info(const srsran_slot_cfg_t& slot_cfg, const pucch_info_t& pucch_info) override;
  int  pusch_info(const srsran_slot_cfg_t& slot_cfg, pusch_info_t& pusch_info) override;
  void rach_detected(const rach_info_t& rach_info) override;

  // X2 interface

  // control plane, i.e. rrc_nr_interface_rrc
  int sgnb_addition_request(uint16_t eutra_rnti, const sgnb_addition_req_params_t& params) final
  {
    return rrc.sgnb_addition_request(eutra_rnti, params);
  };
  int sgnb_reconfiguration_complete(uint16_t eutra_rnti, asn1::dyn_octstring reconfig_response) final
  {
    return rrc.sgnb_reconfiguration_complete(eutra_rnti, reconfig_response);
  };
  // X2 data interface
  void write_sdu(uint16_t rnti, uint32_t lcid, srsran::unique_byte_buffer_t sdu, int pdcp_sn = -1) final
  {
    pdcp.write_sdu(rnti, lcid, std::move(sdu), pdcp_sn);
  }
  std::map<uint32_t, srsran::unique_byte_buffer_t> get_buffered_pdus(uint16_t rnti, uint32_t lcid) final
  {
    return pdcp.get_buffered_pdus(rnti, lcid);
  }

private:
  void run_thread() final;
  void tti_clock_impl();

  // args
  srsenb::stack_args_t    args = {};
  phy_interface_stack_nr* phy  = nullptr;

  srslog::basic_logger& rrc_logger;
  srslog::basic_logger& mac_logger;
  srslog::basic_logger& rlc_logger;
  srslog::basic_logger& pdcp_logger;
  srslog::basic_logger& stack_logger;

  // task scheduling
  static const int                      STACK_MAIN_THREAD_PRIO = 4;
  srsran::task_scheduler                task_sched;
  srsran::task_multiqueue::queue_handle sync_task_queue, ue_task_queue, gw_task_queue, mac_task_queue;

  // derived
  srsenb::mac_nr mac;
  srsenb::rlc    rlc;
  srsenb::pdcp   pdcp;
  srsenb::rrc_nr rrc;
  // std::unique_ptr<sdap> m_sdap;

  // state
  std::atomic<bool> running = {false};
};

} // namespace srsenb

#endif // SRSRAN_GNB_STACK_NR_H
