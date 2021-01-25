/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#ifndef SRSUE_RRC_NR_H
#define SRSUE_RRC_NR_H

#include "srslte/asn1/rrc_nr.h"
#include "srslte/asn1/rrc_nr_utils.h"
#include "srslte/common/block_queue.h"
#include "srslte/common/buffer_pool.h"
#include "srslte/interfaces/nr_common_interface_types.h"
#include "srslte/interfaces/ue_interfaces.h"
#include "srslte/interfaces/ue_nr_interfaces.h"
#include "srsue/hdr/stack/upper/gw.h"

namespace srsue {

// Expert arguments to create GW without proper RRC
struct core_less_args_t {
  std::string      ip_addr;
  srsue::gw_args_t gw_args;
  uint8_t          drb_lcid;
};

struct rrc_nr_args_t {
  core_less_args_t      coreless;
  std::vector<uint32_t> supported_bands;
  std::string           log_level;
  uint32_t              log_hex_limit;
};

struct rrc_nr_metrics_t {};

class rrc_nr final : public rrc_interface_phy_nr,
                     public rrc_interface_pdcp,
                     public rrc_interface_rlc,
                     public rrc_nr_interface_rrc,
                     public srslte::timer_callback
{
public:
  rrc_nr(srslte::task_sched_handle task_sched_);
  ~rrc_nr();

  void init(phy_interface_rrc_nr*       phy_,
            mac_interface_rrc_nr*       mac_,
            rlc_interface_rrc*          rlc_,
            pdcp_interface_rrc*         pdcp_,
            gw_interface_rrc*           gw_,
            rrc_eutra_interface_rrc_nr* rrc_eutra_,
            usim_interface_rrc_nr*      usim_,
            srslte::timer_handler*      timers_,
            stack_interface_rrc*        stack_,
            const rrc_nr_args_t&        args_);

  void stop();
  void init_core_less();

  void get_metrics(rrc_nr_metrics_t& m);

  // Timeout callback interface
  void timer_expired(uint32_t timeout_id) final;
  void srslte_rrc_log(const char* str);

  enum direction_t { Rx = 0, Tx };
  template <class T>
  void log_rrc_message(const std::string&           source,
                       direction_t                  dir,
                       const srslte::byte_buffer_t* pdu,
                       const T&                     msg,
                       const std::string&           msg_type);
  template <class T>
  void log_rrc_message(const std::string&  source,
                       direction_t         dir,
                       asn1::dyn_octstring oct,
                       const T&            msg,
                       const std::string&  msg_type);
  // PHY interface
  void in_sync() final;
  void out_of_sync() final;

  // MAC interface
  void run_tti(uint32_t tti) final;

  // RLC interface
  void max_retx_attempted() final;

  // PDCP interface
  void write_pdu(uint32_t lcid, srslte::unique_byte_buffer_t pdu) final;
  void write_pdu_bcch_bch(srslte::unique_byte_buffer_t pdu) final;
  void write_pdu_bcch_dlsch(srslte::unique_byte_buffer_t pdu) final;
  void write_pdu_pcch(srslte::unique_byte_buffer_t pdu) final;
  void write_pdu_mch(uint32_t lcid, srslte::unique_byte_buffer_t pdu) final;

  // RRC (LTE) interface
  void get_eutra_nr_capabilities(srslte::byte_buffer_t* eutra_nr_caps);
  void get_nr_capabilities(srslte::byte_buffer_t* eutra_nr_caps);
  void phy_meas_stop();
  void phy_set_cells_to_meas(uint32_t carrier_freq_r15);
  bool rrc_reconfiguration(bool                endc_release_and_add_r15,
                           bool                nr_secondary_cell_group_cfg_r15_present,
                           asn1::dyn_octstring nr_secondary_cell_group_cfg_r15,
                           bool                sk_counter_r15_present,
                           uint32_t            sk_counter_r15,
                           bool                nr_radio_bearer_cfg1_r15_present,
                           asn1::dyn_octstring nr_radio_bearer_cfg1_r15);
  void configure_sk_counter(uint16_t sk_counter);
  bool is_config_pending();
  // STACK interface
  void cell_search_completed(const rrc_interface_phy_lte::cell_search_ret_t& cs_ret, const phy_cell_t& found_cell);

private:
  srslte::task_sched_handle task_sched;
  struct cmd_msg_t {
    enum { PDU, PCCH, PDU_MCH, RLF, PDU_BCCH_DLSCH, STOP } command;
    srslte::unique_byte_buffer_t pdu;
    uint16_t                     lcid;
  };

  bool                           running = false;
  srslte::block_queue<cmd_msg_t> cmd_q;

  phy_interface_rrc_nr*       phy       = nullptr;
  mac_interface_rrc_nr*       mac       = nullptr;
  rlc_interface_rrc*          rlc       = nullptr;
  pdcp_interface_rrc*         pdcp      = nullptr;
  gw_interface_rrc*           gw        = nullptr;
  rrc_eutra_interface_rrc_nr* rrc_eutra = nullptr;
  usim_interface_rrc_nr*      usim      = nullptr;
  stack_interface_rrc*        stack     = nullptr;

  srslte::log_ref log_h;

  uint32_t                            fake_measurement_carrier_freq_r15;
  srslte::timer_handler::unique_timer fake_measurement_timer;

  /// RRC states (3GPP 38.331 v15.5.1 Sec 4.2.1)
  enum rrc_nr_state_t {
    RRC_NR_STATE_IDLE = 0,
    RRC_NR_STATE_CONNECTED,
    RRC_NR_STATE_CONNECTED_INACTIVE,
    RRC_NR_STATE_N_ITEMS,
  };
  const static char* rrc_nr_state_text[RRC_NR_STATE_N_ITEMS];

  //  rrc_nr_state_t state = RRC_NR_STATE_IDLE;

  rrc_nr_args_t args = {};

  // RRC constants and timers
  srslte::timer_handler* timers = nullptr;

  std::string get_rb_name(uint32_t lcid) final { return srslte::to_string((srslte::rb_id_nr_t)lcid); }

  typedef enum { Srb = 0, Drb } rb_type_t;
  typedef struct {
    uint32_t  rb_id;
    rb_type_t rb_type;
  } rb_t;

  bool     add_lcid_rb(uint32_t lcid, rb_type_t rb_type, uint32_t rbid);
  uint32_t get_lcid_for_rbid(uint32_t rdid);

  std::map<uint32_t, rb_t> lcid_rb; // Map of lcid to radio bearer (type and rb id)

  std::map<uint32_t, uint32_t> drb_eps_bearer_id; // Map of drb id to eps_bearer_id

  bool apply_cell_group_cfg(const asn1::rrc_nr::cell_group_cfg_s& cell_group_cfg);
  bool apply_radio_bearer_cfg(const asn1::rrc_nr::radio_bearer_cfg_s& radio_bearer_cfg);
  bool apply_rlc_add_mod(const asn1::rrc_nr::rlc_bearer_cfg_s& rlc_bearer_cfg);
  bool apply_mac_cell_group(const asn1::rrc_nr::mac_cell_group_cfg_s& mac_cell_group_cfg);
  bool apply_sp_cell_cfg(const asn1::rrc_nr::sp_cell_cfg_s& sp_cell_cfg);
  bool apply_drb_add_mod(const asn1::rrc_nr::drb_to_add_mod_s& drb_cfg);
  bool apply_security_cfg(const asn1::rrc_nr::security_cfg_s& security_cfg);

  srslte::as_security_config_t sec_cfg;

  class connection_reconf_no_ho_proc
  {
  public:
    explicit connection_reconf_no_ho_proc(rrc_nr* parent_);
    srslte::proc_outcome_t init(const bool                              endc_release_and_add_r15,
                                const asn1::rrc_nr::rrc_recfg_s&        rrc_recfg,
                                const asn1::rrc_nr::cell_group_cfg_s&   cell_group_cfg,
                                bool                                    sk_counter_r15_present,
                                const uint32_t                          sk_counter_r15,
                                const asn1::rrc_nr::radio_bearer_cfg_s& radio_bearer_cfg);
    srslte::proc_outcome_t step() { return srslte::proc_outcome_t::yield; }
    static const char*     name() { return "NR Connection Reconfiguration"; }
    srslte::proc_outcome_t react(const bool& config_complete);
    void                   then(const srslte::proc_state_t& result);

  private:
    // const
    rrc_nr* rrc_ptr;

    asn1::rrc_nr::rrc_recfg_s      rrc_recfg;
    asn1::rrc_nr::cell_group_cfg_s cell_group_cfg;
  };

  srslte::proc_t<connection_reconf_no_ho_proc> conn_recfg_proc;

  srslte::proc_manager_list_t callback_list;
};

} // namespace srsue

#endif // SRSUE_RRC_NR_H