Dir["./commands/*.rb"].each { |file| require_relative file }

class CommandExecutor
  def initialize(input)
    @input = input
  end

  def execute(file_manager)
    @file_manager = file_manager
    command = @input.match(/(?<command>\w+)\s*(?<args>.*)/)
    if command
      execute_command(command[:command], command[:args])
    end
  end

  private
    def execute_command(command, args= nil)
      class_name = command[0, 1].upcase + command[1 .. -1]
      begin
        Commands.const_get(class_name).new(args).execute_with(@file_manager)
      rescue NameError
        raise Commands::InvalidCommandError.new
      end
    end
end
