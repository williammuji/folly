/*
 * Copyright 2017 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/LifoSem.h>
#include <folly/MPMCQueue.h>
#include <folly/executors/BlockingQueue.h>

namespace folly {

template <class T, QueueBehaviorIfFull kBehavior = QueueBehaviorIfFull::THROW>
class LifoSemMPMCQueue : public BlockingQueue<T> {
 public:
  // Note: The queue pre-allocates all memory for max_capacity
  explicit LifoSemMPMCQueue(size_t max_capacity) : queue_(max_capacity) {}

  void add(T item) override {
    switch (kBehavior) { // static
      case QueueBehaviorIfFull::THROW:
        if (!queue_.write(std::move(item))) {
          throw QueueFullException("LifoSemMPMCQueue full, can't add item");
        }
        break;
      case QueueBehaviorIfFull::BLOCK:
        queue_.blockingWrite(std::move(item));
        break;
    }
    sem_.post();
  }

  T take() override {
    T item;
    while (!queue_.readIfNotEmpty(item)) {
      sem_.wait();
    }
    return item;
  }

  size_t capacity() {
    return queue_.capacity();
  }

  size_t size() override {
    return queue_.size();
  }

 private:
  folly::LifoSem sem_;
  folly::MPMCQueue<T> queue_;
};

} // namespace folly