
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

#include <jaegertracing/Tracer.h>

#include "OpenTracingHelpers.h"
#include "gtest/gtest.h"

TEST(OpenTracingHelpersTest, createClearMessage) {
    jaegertracing::Config config(false);
    auto tracer = jaegertracing::Tracer::make("test", config, jaegertracing::logging::nullLogger());
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("createClearMessage");

    ClrMsg created = ClrMsg("msg", "from", "to", 1, 0, 0, traceIdFromContext(span->context()),
                            spanIdFromContext(span->context()));

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&span->context());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().low(), created.getTraceId());
    EXPECT_EQ(jaegerCtx->spanID(), created.getSpanId());

    span->Finish();
    tracer->Close();
}

TEST(OpenTracingHelpersTest, createEncryptedPackage) {
    jaegertracing::Config config(false);
    auto tracer = jaegertracing::Tracer::make("test", config, jaegertracing::logging::nullLogger());
    std::shared_ptr<opentracing::Span> span = tracer->StartSpan("createEncryptedPackage");

    EncPkg created = EncPkg(traceIdFromContext(span->context()), spanIdFromContext(span->context()),
                            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&span->context());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().low(), created.getTraceId());
    EXPECT_EQ(jaegerCtx->spanID(), created.getSpanId());

    span->Finish();
    tracer->Close();
}

TEST(OpenTracingHelpersTest, spanContextFromClrMsg1) {
    ClrMsg message("msg", "from", "to", 1, 0, 0, 1234567890, 987654321);

    std::unique_ptr<opentracing::SpanContext> ctx = spanContextFromClrMsg(message);

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(ctx.get());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().high(), 0);
    EXPECT_EQ(jaegerCtx->traceID().low(), 1234567890);
    EXPECT_EQ(jaegerCtx->spanID(), 987654321);
}

TEST(OpenTracingHelpersTest, spanContextFromClrMsg2) {
    ClrMsg message("msg", "from", "to", 1, 0, 0, 1234567890, 987654321);

    std::unique_ptr<opentracing::SpanContext> ctx = spanContextFromClrMsg(message);

    jaegertracing::Config config(false);
    auto tracer = jaegertracing::Tracer::make("test", config, jaegertracing::logging::nullLogger());
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("spanContextFromClrMsg2", {opentracing::ChildOf(ctx.get())});

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&span->context());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().low(),
              1234567890);                      // trace id should match original trace id
    EXPECT_NE(jaegerCtx->spanID(), 987654321);  // span id should be different

    span->Finish();
    tracer->Close();
}

TEST(OpenTracingHelpersTest, spanContextFromEncryptedPackage1) {
    EncPkg package(1234567890, 987654321, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    std::unique_ptr<opentracing::SpanContext> ctx = spanContextFromEncryptedPackage(package);

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(ctx.get());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().high(), 0);
    EXPECT_EQ(jaegerCtx->traceID().low(), 1234567890);
    EXPECT_EQ(jaegerCtx->spanID(), 987654321);
}

TEST(OpenTracingHelpersTest, spanContextFromEncryptedPackage2) {
    EncPkg package(1234567890, 987654321, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    std::unique_ptr<opentracing::SpanContext> ctx = spanContextFromEncryptedPackage(package);

    jaegertracing::Config config(false);
    auto tracer = jaegertracing::Tracer::make("test", config, jaegertracing::logging::nullLogger());
    std::shared_ptr<opentracing::Span> span =
        tracer->StartSpan("spanContextFromClrMsg2", {opentracing::ChildOf(ctx.get())});

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&span->context());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(jaegerCtx->traceID().low(),
              1234567890);                      // trace id should match original trace id
    EXPECT_NE(jaegerCtx->spanID(), 987654321);  // span id should be different

    span->Finish();
    tracer->Close();
}

TEST(OpenTracingHelpersTest, integrationTest) {
    jaegertracing::Config config(false);
    auto tracer = jaegertracing::Tracer::make("test", config, jaegertracing::logging::nullLogger());
    std::shared_ptr<opentracing::Span> span1 = tracer->StartSpan("integrationTest 1");

    ClrMsg msg = ClrMsg("msg", "from", "to", 1, 0, 0, traceIdFromContext(span1->context()),
                        spanIdFromContext(span1->context()));
    auto ctx1 = spanContextFromClrMsg(msg);
    std::shared_ptr<opentracing::Span> span2 =
        tracer->StartSpan("integrationTest 2", {opentracing::ChildOf(ctx1.get())});

    EncPkg created = EncPkg(traceIdFromContext(span2->context()),
                            spanIdFromContext(span2->context()), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    const auto *jaegerCtx = dynamic_cast<const jaegertracing::SpanContext *>(&span1->context());
    ASSERT_NE(jaegerCtx, nullptr);
    EXPECT_EQ(created.getTraceId(),
              jaegerCtx->traceID().low());  // trace id should match original trace id
    EXPECT_NE(created.getSpanId(),
              jaegerCtx->spanID());  // span id should be different

    span1->Finish();
    span2->Finish();
    tracer->Close();
}
