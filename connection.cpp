//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <functional>
#include <iostream>
#include "transfer.hpp"

#include "yield.hpp"

namespace awesome {

connection::connection(tcp::socket&& down_socket)
  : down_socket_(std::make_shared<tcp::socket>(std::move(down_socket)))
{
}

void connection::operator()(boost::system::error_code ec,
    const tcp::endpoint& up_endpoint)
{
  bool is_stopped = !down_socket_->is_open() && !up_socket_->is_open();
  if (!is_stopped)
  {
    reenter (this)
    {
      std::cout << "connection::start()" << std::endl;

      allocator_ = std::make_shared<allocator>();

      up_socket_ = std::make_shared<tcp::socket>(down_socket_->get_io_service());
      yield up_socket_->async_connect(up_endpoint, *this);
      if (!ec)
      {
        fork connection(*this)(boost::system::error_code());

        buffer_ = std::make_shared<std::array<unsigned char, 1024>>();

        if (is_child())
        {
          allocator_ = std::make_shared<allocator>();

          yield async_transfer(*up_socket_, *down_socket_,
              boost::asio::buffer(*buffer_), *this);
        }
        else
        {
          yield async_transfer(*down_socket_, *up_socket_,
              boost::asio::buffer(*buffer_), *this);
        }
      }

      std::cout << "connection::stop()" << std::endl;

      up_socket_->close(ec);
      down_socket_->close(ec);
    }
  }
}

} // namespace awesome

#include "unyield.hpp"
