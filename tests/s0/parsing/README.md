This directory contains test cases that let each language implementation verify
their S₀ parsers.  Each file should contain one or more YAML documents.  Each
document defines one test case.  There are two possible kinds of test case in
this directory:

- If the YAML mapping has the `!s0!successful-parse` tag, then the mapping
  MUST have a `module` element, which MUST be a valid S₀ module.  Your test
  harness MUST verify that your implementation can parse this module
  successfully.

- If the YAML mapping has the `!s0!invalid-parse` tag, then the mapping MUST
  have a `module` element, which MUST NOT be a valid S₀ module.  If the
  `module` element is a mapping, then it MUST be valid YAML, but there should be
  some logic error in its content that means that it's not a valid S₀ moduel.  If
  the `module` element is a scalar, then the scalar content should containing
  some invalid YAML.

  In both cases, the outer mapping MUST have an `error` element, which contains
  the error message that should be produced when you try to parse the invalid
  module.

  Your test harness MUST verify that you get an error when trying to parse the
  module.  Your test harness MAY further verify that your parser generates the
  expected error message.

Both kinds of test case MUST have a `name` element that describes what the test
case is checking.  If this test case passes in your test harness, you MUST use
this as the description for the TAP message that you produce.
