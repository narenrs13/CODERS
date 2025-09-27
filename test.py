from agent.llm_parser import LLMLocalParser
import json

def main():
    parser = LLMLocalParser()
    print("🤖 Web Navigator AI Agent (type 'exit' to quit)\n")

    while True:
        user_input = input("You: ").strip()
        if user_input.lower() in ["exit", "quit", "bye"]:
            print("Agent: Goodbye! 👋")
            break

        result_json = parser.explain_to_json(user_input)
        print("Agent JSON Response:")
        print(json.dumps(result_json, indent=2), "\n")

if __name__ == "__main__":
    main()
