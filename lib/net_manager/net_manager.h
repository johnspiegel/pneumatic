#ifndef NET_MANAGER_H
#define NET_MANAGER_H

namespace net_manager {

bool Connect(unsigned long timeout_ms);
void DoTask(void* unused);

}  // namespace net_manager

#endif  // NET_MANAGER_H
