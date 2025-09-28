# File: agent/web_navigator_agent.py

import re
import json
from .llm_parser import LLMLocalParser, DEFAULT_SYSTEM_PROMPT
from .browser_automator import StealthBrowserAutomator

# System prompt for the planning LLM
PLANNER_SYSTEM_PROMPT = (
    "You are an AI planner. Read the user instruction carefully and return a clear step-by-step plan in JSON format. "
    "The JSON MUST have a single key 'steps', which is a list of action dictionaries. "
    "Supported actions are 'navigate', 'fill', 'click', and 'extract'. "
    "⚠️ IMPORTANT: Output ONLY valid JSON, no explanations, no code fences."
)

# Helper: Safely extract JSON even if LLM adds extra text/markdown
def safe_parse_json(llm_output: str) -> dict:
    match = re.search(r"\{[\s\S]*\}", llm_output)
    if match:
        try:
            return json.loads(match.group(0))
        except Exception as e:
            print(f"⚠️ JSON parse error after extraction: {e}")
            return {"steps": []}
    else:
        print("⚠️ No JSON found in LLM output")
        return {"steps": []}


class WebNavigatorAgent:
    """
    The Orchestrator middleware. It connects the LLM’s plan 
    to the browser automation tool, enabling multi-step reasoning.
    """

    def __init__(self, llm_model="llama3", headless=False):
        # Initialize LLM Planner
        self.parser = LLMLocalParser(
            model_name=llm_model,
            system_prompt=DEFAULT_SYSTEM_PROMPT
        )

        # Initialize Browser Automator
        self.automator = StealthBrowserAutomator(headless=headless)

        # Memory module for task history
        self.memory = {}
        self.planner_system_prompt = PLANNER_SYSTEM_PROMPT

    def generate_plan(self, instruction: str) -> dict:
        """Uses the LLM to convert the user instruction into a structured JSON plan."""
        planner_question = (
            f"INSTRUCTION:\n{instruction}\n\n"
            "Generate the plan as a JSON object with a single top-level key 'steps'."
        )

        # Temporarily change system prompt for planning
        original_prompt = self.parser.system_prompt
        self.parser.system_prompt = self.planner_system_prompt

        # Generate the structured plan
        plan_response = self.parser.explain_to_json(planner_question)

        # Restore original prompt
        self.parser.system_prompt = original_prompt

        try:
            plan = safe_parse_json(plan_response['answer'])
            return plan
        except Exception as e:
            print(f"❌ Failed to parse JSON plan from LLM: {e}")
            print(f"LLM Raw Output: {plan_response.get('answer', 'N/A')}")
            return {"steps": []}

    def execute_plan(self, plan: dict) -> str:
        """
        Executes the steps from a flat JSON plan using the browser automator.
        Supports keys: url, field, element, target.
        """
        results = {}
        if not plan.get("steps"):
            return "Execution failed: No valid plan to execute."

        print("\n--- Starting Web Automation ---")
        self.automator.start()  # Starts the browser

        try:
            for i, step in enumerate(plan['steps']):
                action = step.get('action')
                print(f"\n[STEP {i+1}] Action: {action}")

                if action == 'navigate':
                    url = step.get('url')
                    if not url:
                        print(f"❌ ERROR: URL missing in navigate step: {step}")
                        continue
                    self.automator.navigate(url)

                elif action == 'fill':
                    selector = step.get('field') or step.get('selector')
                    value = step.get('value', '')
                    if not selector:
                        print(f"❌ ERROR: Selector missing in fill step: {step}")
                        continue
                    self.automator.fill_form(selector, value)

                elif action == 'click':
                    selector = step.get('element') or step.get('selector')
                    if not selector:
                        print(f"❌ ERROR: Selector missing in click step: {step}")
                        continue
                    self.automator.click_element(selector)

                elif action == 'extract':
                    selector = step.get('target') or step.get('selector')
                    if not selector:
                        print(f"❌ ERROR: Selector missing in extract step: {step}")
                        continue

                    extract_kwargs = {"attribute": "text"}
                    limit = step.get("limit")
                    if limit:
                        extract_kwargs["limit"] = limit

                    data = self.automator.extract_data(selector, **extract_kwargs)
                    result_key = step.get('result_key', f'step_{i}_data')
                    results[result_key] = data
                    print(f"✅ Stored {len(data)} items under key: {result_key}")

                elif action in ["filter", "sort"]:
                    print(f"⚠️ Unknown action: {action}. Skipping.")

        except Exception as e:
            print(f"🚨 Execution Error during step {i+1}: {e}")

        finally:
            self.automator.close()

        return json.dumps(results, indent=2)

    def run(self, instruction: str):
        """Main entry point for the agent."""
        print(f"👤 User Instruction: {instruction}")
        plan = self.generate_plan(instruction)
        print("💡 Generated Plan:\n", json.dumps(plan, indent=2))

        final_results = self.execute_plan(plan)

        self.memory[instruction] = final_results

        print("\n--- FINAL RESULT ---")
        print(final_results)
        return final_results
