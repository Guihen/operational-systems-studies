module Commands
  class Command
    class AbstractMethodError < RuntimeError; end
    class InvalidArgumentsError < RuntimeError; end

    attr_reader :args

    def initialize(args)
      @args = args
    end

    def execute_with(file_manager)
      @file_manager = file_manager
      execute
    end

    protected
      def execute
        raise AbstractMethodError.new
      end

      def get_two_parmeters!
        parameters = args.scan(/\S+/)
        if parameters.size == 2
          parameters.slice(0, 2)
        else
          raise InvalidArgumentsError.new
        end
      end
  end
end
