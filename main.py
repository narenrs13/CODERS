from agent.web_navigator_agent import WebNavigatorAgent

def main():
    # Initialize the agent. Set headless=False to watch the browser in action.
    agent = WebNavigatorAgent(headless=False) 
    print("🤖 Web Navigator AI Agent Initialized. Ready for task.")

    # Example Task for Demonstration
    user_task = "Go to Google, search for 'latest tech news', and extract the titles of the first 5 results."

    # Run the agent pipeline (Plan -> Execute -> Output)
    agent.run(user_task)

    # You can inspect the memory module after running
    # print("\nAgent Memory Snapshot:", agent.memory)
    print("\nTask Execution Complete. Final Result:")
    print(final_result)

    print("\n🔍 Agent Memory Snapshot:")
    print(agent.memory)


if __name__ == "__main__":
    main()