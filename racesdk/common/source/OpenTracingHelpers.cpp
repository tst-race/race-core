
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

#include "OpenTracingHelpers.h"

#include <RaceLog.h>
#include <jaegertracing/SpanContext.h>
#include <jaegertracing/Tracer.h>

#include <filesystem>
#include <fstream>

/**
 * @brief Helper function to generate a opentracing::SpanContext from a given trace ID and span ID.
 * The returned object is an instance of jaegertracing::SpanContext.
 *
 * @param traceId The trace ID to be used to generate the SpanContext.
 * @param spanId The span ID to be used to generate the SpanContext.
 * @return std::unique_ptr<opentracing::SpanContext> The newly created object.
 */
static inline std::unique_ptr<opentracing::SpanContext> spanContextFromTraceIdAndSpanId(
    const uint64_t traceId, const uint64_t spanId) {
    return std::unique_ptr<opentracing::SpanContext>(new jaegertracing::SpanContext(
        jaegertracing::TraceID(0, traceId), spanId, 0,
        static_cast<unsigned char>(jaegertracing::SpanContext::Flag::kSampled), {}));
}

std::unique_ptr<opentracing::SpanContext> spanContextFromClrMsg(const ClrMsg &msg) {
    return spanContextFromTraceIdAndSpanId(msg.getTraceId(), msg.getSpanId());
}

std::unique_ptr<opentracing::SpanContext> spanContextFromEncryptedPackage(const EncPkg &pkg) {
    return spanContextFromTraceIdAndSpanId(pkg.getTraceId(), pkg.getSpanId());
}

std::unique_ptr<opentracing::SpanContext> spanContextFromIds(
    const std::pair<std::uint64_t, std::uint64_t> &ids) {
    return spanContextFromTraceIdAndSpanId(ids.first, ids.second);
}

uint64_t traceIdFromContext(const opentracing::SpanContext &ctx) {
    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&ctx);
    if (jaegerCtx != nullptr) {
        return jaegerCtx->traceID().low();
    }

    RaceLog::logError("OpenTracingHelpers", "Failed to get traceId from context", "");
    return 0;
}

uint64_t spanIdFromContext(const opentracing::SpanContext &ctx) {
    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&ctx);
    if (jaegerCtx != nullptr) {
        return jaegerCtx->spanID();
    }

    RaceLog::logError("OpenTracingHelpers", "Failed to get spanId from context", "");
    return 0;
}

std::shared_ptr<opentracing::Tracer> createTracer(const std::string &jaegerConfigPath,
                                                  std::string persona) {
    TRACE_FUNCTION_BASE(RaceSdkCore, jaegerConfigPath, persona);
    try {
        // disable jaeger if there's no path
        jaegertracing::Config config(true);
        if (!jaegerConfigPath.empty() and std::filesystem::exists(jaegerConfigPath)) {
            auto configYAML = YAML::LoadFile(jaegerConfigPath);
            config = jaegertracing::Config::parse(configYAML);
        }

        if (persona.empty()) {
            RaceLog::logError(logPrefix, "persona is empty", "");
            persona = "unknown";
        }

        return jaegertracing::Tracer::make(persona, config,
                                           jaegertracing::logging::consoleLogger());
    } catch (std::runtime_error &e) {
        const std::string errorMessage =
            "Error: Failed to initialize OpenTracing using '" + jaegerConfigPath + "':" + e.what();
        RaceLog::logError("OpenTracingHelpers", errorMessage, "");
        throw std::invalid_argument(errorMessage);
    }
}
