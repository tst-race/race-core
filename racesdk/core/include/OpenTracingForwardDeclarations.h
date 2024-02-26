
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

#ifndef __OPEN_TRACING_FORWARD_DECLARATIONS_H__
#define __OPEN_TRACING_FORWARD_DECLARATIONS_H__

// forward declarations for opentracing to prevent having to include the header
namespace opentracing {
inline namespace v3 {
class Span;
class SpanContext;
class Tracer;
}  // namespace v3
using Span = v3::Span;
using SpanContext = v3::SpanContext;
using Tracer = v3::Tracer;
}  // namespace opentracing

#endif