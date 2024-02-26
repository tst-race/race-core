
//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef __OPENTRACING_HELPERS_
#define __OPENTRACING_HELPERS_

#include <opentracing/span.h>
#include <opentracing/tracer.h>

#include <memory>

#include "ClrMsg.h"
#include "EncPkg.h"
#include "IRaceSdkCommon.h"

/**
 * @brief Create a new opentracing::SpanContext using a clear messages's trace ID and span ID. The
 * returned object is an instance of jaegertracing::SpanContext.
 *
 * @param msg The message whose trace ID and span ID are used to create the SpanContext.
 * @return std::unique_ptr<opentracing::SpanContext> The newly created object.
 */
std::unique_ptr<opentracing::SpanContext> spanContextFromClrMsg(const ClrMsg &msg);

/**
 * @brief Create a new opentracing::SpanContext using an encrypted package's trace ID and span ID.
 * The returned object is an instance of jaegertracing::SpanContext.
 *
 * @param msg The encrypted package whose trace ID and span ID are used to create the SpanContext.
 * @return std::unique_ptr<opentracing::SpanContext> The newly created object.
 */
std::unique_ptr<opentracing::SpanContext> spanContextFromEncryptedPackage(const EncPkg &pkg);

/**
 * @brief Create a new opentracing::SpanContext using the provided (traceId,spanId) pair.
 * The returned object is an instance of jaegertracing::SpanContext.
 *
 * @param ids A traceId,spanId pair
 */
std::unique_ptr<opentracing::SpanContext> spanContextFromIds(
    const std::pair<std::uint64_t, std::uint64_t> &ids);

/**
 * @brief Get the trace ID from a given SpanContext. Note that this will return the lower 64 bits of
 * the jaegertracing::TraceID object. Also note that if the underlying type of the SpanContext is
 * not an instance of jaegertracing::SpanContext then this function will return zero, indicating
 * error.
 *
 * @param ctx The SpanContext from which to retrieve the trace ID.
 * @return uint64_t The trace ID, or zero on error.
 */
uint64_t traceIdFromContext(const opentracing::SpanContext &ctx);

/**
 * @brief Get the span ID from a given SpanContext. Note that if the underlying type of the
 * SpanContext is not an instance of jaegertracing::SpanContext then this function will return zero,
 * indicating error.
 *
 * @param ctx The SpanContext from which to retrieve the span ID.
 * @return uint64_t The span ID, or zero on error.
 */
uint64_t spanIdFromContext(const opentracing::SpanContext &ctx);

/**
 * @brief Create a opentracing tracer. The tracer must be created by this method, because android
 * does not allow dynamic casts across library boundaries. Throws invalid argument exception if the
 * config file is not valid.
 *
 * @param jaegerConfigPath The path to read the config from
 * @param persona The name to use when logging to open tracing
 * @return shared ptr to the tracer
 */
std::shared_ptr<opentracing::Tracer> createTracer(const std::string &jaegerConfigPath,
                                                  std::string persona);

#endif
