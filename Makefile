.PHONY: help test coverage coverage-report clean

help:
	@echo "Iometer Build Targets"
	@echo "===================="
	@echo ""
	@echo "test              - Run unit tests"
	@echo "coverage          - Generate code coverage report (requires cmake, ninja, LLVM)"
	@echo "coverage-report   - Generate interactive HTML coverage report"
	@echo "clean             - Clean build artifacts"
	@echo ""

test:
	@echo "Running unit tests..."
	cd src/qt/build/Release && \
	for test in tst_types tst_accessspecs tst_icf tst_results tst_demo tst_protocol; do \
		echo "Running $$test..."; \
		./$$test.exe -v2 || true; \
	done

coverage-report:
	@echo "Generating interactive coverage report..."
	cd src/qt && \
	powershell -ExecutionPolicy Bypass -File generate_coverage_report.ps1 -OutputDir "../../coverage_report" && \
	echo "" && \
	echo "Report generated: coverage_report/index.html" && \
	echo "To view: open coverage_report/index.html in your browser"

coverage:
	@echo "Building with coverage instrumentation (requires cmake, ninja, LLVM)..."
	cd src/qt && \
	powershell -ExecutionPolicy Bypass -File collect_coverage.ps1 && \
	echo "" && \
	echo "Coverage report generated: build_coverage/coverage_report/index.html"

clean:
	@echo "Cleaning build artifacts..."
	cd src/qt && \
	rm -rf build build_coverage coverage_report 2>/dev/null || true && \
	echo "Clean complete"

# Advanced targets
.PHONY: build test-all coverage-full
build:
	@echo "Building Qt application..."
	cd src/qt && cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja && cmake --build build

test-all:
	@echo "Running all tests (including UI tests if display available)..."
	cd src/qt/build/Release && \
	for test in tst_types tst_accessspecs tst_icf tst_results tst_demo tst_protocol tst_meters tst_pagesetup tst_mainwindow; do \
		echo "Running $$test..."; \
		./$$test.exe -v2 || true; \
	done

coverage-full: coverage coverage-report
	@echo "Full coverage generation complete"
