# test_plan_runner.py
from agent.web_navigator_agent import WebNavigatorAgent   # or from web_navigator_agent import ...
import json

if __name__ == "__main__":
    agent = WebNavigatorAgent(headless=False)
    # Simple plan that mirrors the automator's run_tests (DuckDuckGo search)
    plan = {
        "steps": [
            {"action": "navigate", "details": {"url": "https://duckduckgo.com"}},
            {"action": "fill", "details": {"selector": "#search_form_input_homepage", "value": "best travel cameras 2024 review"}},
            {"action": "click", "details": {"selector": "#search_button_homepage"}},
            {"action": "extract", "details": {"selector": "a.result__a", "attribute": "text", "limit": 5, "result_key": "top_results"}}
        ]
    }
    print(agent.execute_plan(plan))
