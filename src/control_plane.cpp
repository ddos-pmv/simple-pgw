#include <control_plane.h>

#include "teid_generator.h"

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    return _pdns.contains(cp_teid) ? _pdns[cp_teid] : nullptr;
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    return _pdns_by_ue_ip_addr.contains(ip) ? _pdns_by_ue_ip_addr[ip] : nullptr;
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
        return _bearers.contains(dp_teid) ? _bearers[dp_teid] : nullptr}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
                                                                     boost::asio::ip::address_v4 sgw_addr,
                                                                     uint32_t sgw_cp_teid) {


    if (!_apns.contains(apn))
        return nullptr;

    uint32_t pgw_cp_teid = TeidGenerator::generate();
    uint32_t pgw_dp_teid = TeidGenerator::generate();

    auto pdn_con = pdn_connection::create(pgw_cp_teid, _apns[apn], allocate_ue_ip());

    pdn_con->set_sgw_cp_teid(pgw_cp_teid);
    pdn_con->set_sgw_addr(sgw_addr);

    _pdns[cp_teid] = pdn_con;
    _pdns_by_ue_ip_addr[ue_ip] = pdn_con;

    return pdn_con;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    if (_pdns.contains(cp_teid)) {
        auto pdn_to_delete = _pdns[cp_teid];
        for (const auto &[dp_teid, bearer]: pdn_to_delete->_bearers) {
            _bearers.erase(bearer);
        }

        _pdns_by_ue_ip_addr.erase(pdn_to_delete->get_ue_ip_addr());
        _pdns.erase(cp_teid);
    }

    TeidGenerator::release(cp_teid);
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    if (!pdn.get())
        return nullptr;

    uint32_t dp_teid = TeidGenerator::generate();
    auto b = std::make_shared<bearer>(dp_teid, sgw_teid);

    pdn->add_bearer(b);
    _bearers[dp_teid] = b;

    return b;
}

void control_plane::delete_bearer(uint32_t dp_teid) {
    auto it = _bearers.find(dp_teid);
    if (it != _bearers.end()) {
        auto bearer = it->second;
        auto pdn = bearer->get_pdn_connection();
        if (pdn)
            pdn->remove_bearer(dp_teid);
        _bearers.erase(it);
        TeidGenerator::release(dp_teid);
    }
}

void control_plane::add_apn(const std::string &apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns[apn_name] = std::move(apn_gateway);
}

boost::asio::ip::address_v4 control_plane::allocate_ue_ip() {
    static uint32_t ip_pool = (10 << 24) | (0 << 16) | (0 << 8) | 1; // 10.0.0.1
    return boost::asio::ip::address_v4{ip_pool.fetch_add(1)};
}
